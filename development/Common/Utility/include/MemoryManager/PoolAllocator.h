///////////////////////////////////////////////////////////////////////
// PoolAllocator.h
//
//  Copyright 8/10/2017 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Memory pool allocator with compile time memory size
//      Needed header if using memory::New(...)
//      #include <functional>
//
//  #include "MemoryManager/PoolAllocator.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "YagetCore.h"
#include "Metrics/Gather.h"
#include "Debugging/Assert.h"
#include "Exception/Exception.h"
#include <bitset>
#include <array>
#include <memory>


namespace yaget
{
    namespace memory
    {
        namespace internal
        {
            //! Represents one line of memory block. It keeps track which slots
            //! are occupied, by using bitset. T represent object type to manage and E specifies number of slots.
            //! Typical value for that is 32 or 64.
            //! This class is used by PoolAllocator class and not design for external use
            template <typename T, int E>
            class PoolAllocatorLine : public Noncopyable<PoolAllocatorLine<T, E>>
            {
                static_assert(E > 0, "Number of Slots for Allocator Line must be bigger then 0.");

            public:
                static constexpr int INVALID_SLOT = -1;

                struct BlockHeader
                {
                    int mSlotIndex;
                    int mLineIndex;
                };

                ~PoolAllocatorLine()
                {
                    YAGET_ASSERT(mFlags.none(), "PoolAllocatorLine for '%s' still has outstanding '%d' allocation(s).", typeid(T).name(), mFlags.count());
                }

                //! Caller must assure that there is free slot, otherwise this method will assert.
                template<typename... Args>
                T* Allocate(Args&&... args)
                {
                    YM_GATHER_PARENT(LineAllocate);

                    int freeSlot = 0;
                    {
                        YM_GATHER(ClaimFreeSlot);
                        freeSlot = ClaimFreeSlot();
                    }
                    // get the memory where this slot starts
                    char* memoryBlock = &mMemory[freeSlot * kStrideSize];

                    // initialize BlockHeader
                    (void)new(memoryBlock) BlockHeader({ freeSlot, PoolAllocatorLine::INVALID_SLOT });
                    // advance memory passed BlockHeader and use placement new T
                    try
                    {
                        char* objectMemory = memoryBlock + kHeaderSize;

                        T* instance = nullptr;
                        if constexpr (std::is_constructible_v<T, Args...>)
                        {
                            instance = new(objectMemory) T(std::forward<Args>(args)...);
                        }
                        else
                        {
                            // NOTE EG: I could not find a way to silence compiler about
                            // unused args, and did not want to use #pragma warning
                            using Dummies = std::tuple<Args...>;
                            [[maybe_unused]] Dummies dummy(args...);
                            instance = new(objectMemory) T{};
                        }

                        return instance;
                    }
                    catch (const ex::bad_init& e)
                    {
                        ResetSlot(freeSlot);
                        YAGET_ASSERT(false, "Examine this bad_init excpetion: '%s'.", e.what());
                        throw;
                    }
                    catch (const ex::bad_resource& e)
                    {
                        ResetSlot(freeSlot);
                        YAGET_ASSERT(false, "Examine this bad_resource excpetion: '%s'.", e.what());
                        throw;
                    }
                }

                void Free(T* allocatedMemory)
                {
                    BlockHeader* blockHeader = GetBlockHeader(allocatedMemory);
                    YAGET_ASSERT(blockHeader->mSlotIndex != INVALID_SLOT, "Memory slot for '%s' is not marked as allocated.", typeid(T).name());

                    allocatedMemory->~T();
                    mFlags[blockHeader->mSlotIndex] = false;
                    blockHeader->mSlotIndex = INVALID_SLOT;
                }

                bool IsFull() const { return mFlags.all(); }
                bool IsEmpty() const { return mFlags.none(); }

                static BlockHeader* GetBlockHeader(T* instance)
                {
                    YAGET_ASSERT(instance, "Param instance of type '%s' is nullptr.", typeid(T).name());
                    char* objectMemory = reinterpret_cast<char*>(instance);
                    char* memoryBlock = objectMemory - kHeaderSize;
                    BlockHeader* blockHeader = reinterpret_cast<BlockHeader*>(memoryBlock);
                    return blockHeader;
                }

            private:
                void ResetSlot(int slotId)
                {
                    char* memoryBlock = &mMemory[slotId * kStrideSize];
                    BlockHeader* blockHeader = reinterpret_cast<BlockHeader*>(memoryBlock);
                    blockHeader->mSlotIndex = INVALID_SLOT;
                    mFlags[slotId] = false;
                }

                int ClaimFreeSlot()
                {
                    YAGET_ASSERT(!IsFull(), "PoolAllocatorLine is out of memory for '%s'", typeid(T).name());

                    for (size_t i = 0; i < mFlags.size(); ++i)
                    {
                        if (!mFlags[i])
                        {
                            mFlags[i] = true;
                            return static_cast<int>(i);
                        }
                    }

                    YAGET_ASSERT(false, "This code should not have been triggered for '%s'", typeid(T).name());
                    return INVALID_SLOT;
                }

                static constexpr size_t kElementSize = sizeof(T);
                static constexpr size_t kHeaderSize = sizeof(BlockHeader);
                static constexpr size_t kStrideSize = kElementSize + kHeaderSize;
                static constexpr size_t kOverheadSize = E * kHeaderSize;

                // we keep track which slots are allocated and which one are free
                std::bitset<E> mFlags{};
                // memory block representing T's
                // memory layout: BlockHeader|T|BlockHeader|T|...
                // memory returned to user points to T in that slot
                std::array<char, kStrideSize * E> mMemory{};
            };

        } // namespace internal


        //! Provides memory management. It uses memory lines with the same number of slots
        //! and will add new line if all the lines are full.
        //! It does not provide GC and it's up to user to call Free.
        //! T type must have static capacity member:
        //!         static constexpr int Capacity = <number_of_slots_per_line_in_memory_pool>;
        //! TODO: Do we want to add some kind of compaction? This would mean that pointers would need
        //! to be changed,
        template <typename T, int E = T::Capacity>
        class PoolAllocator
        {
        public:
            using Type = T;
            static constexpr int Size = E;

            PoolAllocator() = default;
            ~PoolAllocator() = default;

            template<typename... Args>
            T* Allocate(Args&&... args)
            {
                YM_GATHER_PARENT(PoolAllocate);

                PoolLine_t* currentLine = nullptr;
                {
                    YM_GATHER(FindLine);

                    if (mLastLineIndex == PoolLine_t::INVALID_SLOT)
                    {
                        int lineIndex = PoolLine_t::INVALID_SLOT;

                        // find first available line which is not full yet
                        auto it = std::find_if(mMemoryLines.begin(), mMemoryLines.end(), [](const PoolLinePtr_t& line) { return !line->IsFull(); });
                        if (it == mMemoryLines.end())
                        {
                            mMemoryLines.push_back(std::make_unique<PoolLine_t>());

                            lineIndex = static_cast<int>(mMemoryLines.size() - 1);
                            currentLine = mMemoryLines.back().get();

                        }
                        else
                        {
                            lineIndex = static_cast<int>(std::distance(mMemoryLines.begin(), it));
                            currentLine = it->get();
                        }

                        mLastLineIndex = lineIndex;
                    }
                    else
                    {
                        currentLine = mMemoryLines[mLastLineIndex].get();
                    }
                }

                // this line can throw exception, so none of our bookkeeping code will get executed
                // pass this line if exception throw
                T* instance = currentLine->Allocate(std::forward<Args>(args)...);

                YM_GATHER_PARENT(AllocateLine);
                auto* blockHeader = PoolLine_t::GetBlockHeader(instance);
                blockHeader->mLineIndex = mLastLineIndex;

                // check if current line still has any slots left
                mLastLineIndex = currentLine->IsFull() ? PoolLine_t::INVALID_SLOT : mLastLineIndex;
                return instance;
            }

            void Free(T* allocatedMemory)
            {
                // Potential place to lock (if MT)
                auto* blockHeader = PoolLine_t::GetBlockHeader(allocatedMemory);
                YAGET_ASSERT(blockHeader->mLineIndex != PoolLine_t::INVALID_SLOT, "Invalid Component '%s' deletion (double-delete?).", typeid(T).name());
                mMemoryLines[blockHeader->mLineIndex]->Free(allocatedMemory);
                blockHeader->mLineIndex = PoolLine_t::INVALID_SLOT;

                // TODO: If we want to handle removing empty lines, we will also need to adjust mLineIndex in BlockHeader
            }

        private:
            using PoolLine_t = internal::PoolAllocatorLine<T, E>;
            using PoolLinePtr_t = std::unique_ptr<PoolLine_t>;
            std::vector<PoolLinePtr_t> mMemoryLines;
            int mLastLineIndex = PoolLine_t::INVALID_SLOT;;
        };

        // Helper to create shared pointer with custom deleter
        template <typename T, typename... Args>
        std::shared_ptr<typename T::Type> New(T& poolAllocator, Args&&... args)
        {
            typename T::Type* newObject = poolAllocator.Allocate(std::forward<Args>(args)...);
            auto objecttHandle = std::shared_ptr<T::Type>(newObject, [&poolAllocator](typename T::Type* oldObject)
            {
                poolAllocator.Free(oldObject);
            });
            return objecttHandle;
        }

    } // namespace memory

} // namespace yaget