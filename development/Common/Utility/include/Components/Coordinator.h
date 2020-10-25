//////////////////////////////////////////////////////////////////////
// Coordinator.h
//
//  Copyright 6/16/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides collection and management of items (set of components)
//      This used to be represented by Scene.
//
//
//  #include "Components/Coordinator.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/ComponentTypes.h"
#include "Components/GameCoordinatorGenerator.h"

#include <any>



namespace yaget
{
    namespace comp
    {
        namespace internal
        {
            template<typename T>
            concept has_auto_cleanup = requires { typename T::AutoCleanup; };

        }

        // Coordinator stores map of items (keyed on item guid), manages creation, storage and deletion of components.
        // It uses PoolAllocator as a storage for components.
        template <typename P>
        class Coordinator : public Noncopyable<Coordinator<P>>
        {
        public:
            using Policy = P;
            using Row = typename Policy::Row;

            static constexpr size_t NumComponents = std::tuple_size_v<std::remove_reference_t<Row>>;
            static_assert(NumComponents > 0, "Policy must have at least one Component");

            using PatternSet = std::bitset<NumComponents>;

            Coordinator();
            ~Coordinator();

            // Add component to pool and collection.
            // This will create a new instance of T component allocator
            template<typename T, typename... Args>
            T* AddComponent(comp::Id_t id, Args&&... args);

            // Remove and delete component. It will set component to nullptr
            template<typename T>
            void RemoveComponent(comp::Id_t id, T*& component);

            // Remove and delete component type from item id
            template<typename T>
            void RemoveComponent(comp::Id_t id);

            // Remove all components with this id
            void RemoveComponents(comp::Id_t id);

            // Remove items 
            void RemoveItems(const comp::ItemIds& ids);

            // Return a component for specific item if one exist
            template<typename T>
            T* FindComponent(comp::Id_t id) const;

            // Return full item if one exist
            Row FindItem(comp::Id_t id) const;

            // Return item if one exist but only for specified components in RowPolicy param R (comp::RowPolicy<...>)
            template<typename R>
            typename R::Row FindItem(comp::Id_t id) const;

            // Return item id's for any item that matches components of R
            template<typename R>
            comp::ItemIds GetItemIds() const;

            // Iterate over each item in ids collection and call callback on each item
            template<typename R>
            void ForEach(const comp::ItemIds& ids, std::function<bool(comp::Id_t id, const typename R::Row& row)> callback);

            // Iterate over all items that conform to pattern R
            // Return number of matched items, or 0 if none.
            template<typename R>
            std::size_t ForEach(std::function<bool(comp::Id_t id, const typename R::Row& row)> callback);

            template<typename T>
            PatternSet GetPattern() const
            {
                PatternSet bits = 0;
                auto bp = GetBitPosition<T>();
                bits[bp] = true;

                return std::move(bits);
            }

        private:
            // Helper method to find a specific component allocator
            template<typename T>
            memory::PoolAllocator<T>* FindAllocator() const;

            // finds 'any' allocator. It's up to a user to cast it to correct type and handle any errors
            std::any FindAllocator(const std::type_index& allocId) const;

            template<typename T>
            constexpr uint32_t GetBitPosition() const
            {
                constexpr auto index = meta::Index<T*, Row>::value;
                return index;

                //using CompType = typename std::remove_pointer<std::decay_t<T>>::type();

                //auto it = mItemBitsMapping.find(std::type_index(typeid(CompType)));
                //YAGET_ASSERT(mItemBitsMapping.find(std::type_index(typeid(CompType))) != mItemBitsMapping.end(), "Requested Component: '%s' does not exist in this Coordinator as an Item.", typeid(T).name());

                //constexpr auto index = meta::Index<T*, Row>::value;
                //index;
                //Row row{};
                //row;
                //T* t = nullptr;
                //t;
                //return it->second;
            }

            template<typename R>
            PatternSet MakeBits() const
            {
                using Item = R;
                PatternSet bits = 0;

                meta::for_each_type<Item>([&bits, this](const auto& compType)
                {
                    using CompType = std::decay_t<decltype(*compType)>;
                    bits[GetBitPosition<CompType>()] = true;;
                });

                return bits;
            }

            template<typename... Args>
            PatternSet GetValidBits(const std::tuple<Args...>& item) const
            {
                PatternSet bits = 0;

                meta::for_each(item, [this, &bits](auto& compType)
                {
                    if (compType)
                    {
                        using CompType = std::decay_t<decltype(*compType)>;
                        bits[GetBitPosition<CompType>()] = true;
                    }
                });

                return bits;
            }

            using AllocatorsList = std::map<std::type_index, std::any>;
            AllocatorsList mComponentAllocators;

            // Actual item created from PoolAllocator
            std::map<comp::Id_t, Row> mItems;

            // map from unique bits to all id's which contain that specific set of components
            using Patterns = std::unordered_map<PatternSet, std::set<comp::Id_t>>;
            Patterns mPatterns;

            // This associates each type in a Row with bit position.
            // Used in creating bit pattern of components, 
            // which in turn helps to find a specific set of items.
            using ItemBits = std::unordered_map<std::type_index, uint32_t>;
            ItemBits mItemBitsMapping;

            const Strings mComponentNames;
        };

        namespace internal
        {
            template<int N, typename To>
            void RowCopy(To& to, const To& from)
            {
                to = from;
            }

            template<int N, typename To, typename From>
            void RowCopy(To& to, const From& from)
            {
                auto element = std::get<std::tuple_element<N - 1, To>::type>(from);
                std::get<N - 1>(to) = element;
                if constexpr (N - 1 > 0)
                {
                    RowCopy<N - 1, To, From>(to, from);
                }
            }

        } // namespace internal
        
    } // namespace comp
} // namespace yaget

//----------------------------------------------------------------------------------------------------------------------------------------------------
template<typename P>
yaget::comp::Coordinator<P>::Coordinator()
    : mComponentNames(comp::db::GetPolicyRowNames<P::Row>())
{
    int counter = 0;
    meta::for_each_type<P::Row>([this, &counter](const auto& logType)
    {
        using LogType = typename std::remove_pointer<std::decay_t<decltype(*logType)>>::type();
        mItemBitsMapping.insert(std::make_pair(std::type_index(typeid(LogType)), counter++));
    });
}

template<typename P>
yaget::comp::Coordinator<P>::~Coordinator()
{
    if constexpr (internal::has_auto_cleanup<Policy>)
    {
        YLOG_CWARNING("GSYS", mItems.empty(), "Coordinator [%s] still has outstanding '%d' component(s): [%s]",
            conv::Combine(mComponentNames, ", ").c_str(),
            mItems.size(), 
            conv::Combine(mItems, "], [").c_str());

        comp::ItemIds items;
        for (const auto& [id, row] : mItems)
        {
            items.insert(id);
        }

        RemoveItems(items);
    }
    else
    {
        YAGET_ASSERT(mItems.empty(), "Coordinator still has outstanding '%d' component(s): [%s]", mItems.size(), conv::Combine(mItems, "], [").c_str());
    }
}

template<typename P>
std::any yaget::comp::Coordinator<P>::FindAllocator(const std::type_index& allocI) const
{
    auto it = mComponentAllocators.find(allocI);
    return it != mComponentAllocators.end() ? it->second : std::any();
}

template<typename P>
template<typename T, typename... Args>
T* yaget::comp::Coordinator<P>::AddComponent(comp::Id_t id, Args&&... args)
{
    memory::PoolAllocator<T>* componentAllocator = FindAllocator<T>();
    if (!componentAllocator)
    {
        auto anyAllocator = std::make_shared<memory::PoolAllocator<T>>();
        componentAllocator = anyAllocator.get();
        mComponentAllocators.insert(std::make_pair(std::type_index(typeid(T)), anyAllocator));
    }

    Row row = FindItem(id);
    PatternSet currentBits = GetValidBits(row);
    using NewComp = std::tuple<T*>;
    PatternSet newBit = MakeBits<NewComp>();
    YAGET_ASSERT((currentBits & newBit) != newBit, "Reqested new component of type: '%s' for Item: '%d' already exist in Coordinator.", typeid(T).name(), id);

    if (currentBits.any())
    {
        mPatterns[currentBits].erase(id);
        if (mPatterns[currentBits].empty())
        {
            mPatterns.erase(currentBits);
        }
    }

    T* newComponenet = componentAllocator->Allocate(std::forward<comp::Id_t>(id), std::forward<Args>(args)...);

    std::get<T*>(mItems[id]) = newComponenet;

    Row newRow = FindItem(id);
    PatternSet newBits = GetValidBits(newRow);
    YAGET_ASSERT(newBits.any(), "Adding new Component to an item did not create any new pattern bits.");
    mPatterns[newBits].insert(id);

    return newComponenet;
}

template<typename P>
template<typename T>
void yaget::comp::Coordinator<P>::RemoveComponent(comp::Id_t id, T*& component)
{
    YAGET_ASSERT(component, "Component parameter of type: '%s' is nulptr.", typeid(T).name());
    YAGET_ASSERT(mItems.find(id) != mItems.end(), "Item id: '%d' of type: '%s' does not exist in collection.", id, typeid(T).name());

    memory::PoolAllocator<T>* componentAllocator = FindAllocator<T>();
    YAGET_ASSERT(componentAllocator, "Did not find pool allocotor for: '%s'.", typeid(T).name());

    Row row = FindItem(id);
    PatternSet currentBits = GetValidBits(row);
    mPatterns[currentBits].erase(id);

    componentAllocator->Free(component);
    std::get<T*>(mItems[id]) = nullptr;
    component = nullptr;

    if (mItems[id] == Row())
    {
        mItems.erase(id);
        mPatterns.erase(currentBits);
    }
    else
    {
        Row newRow = FindItem(id);
        PatternSet newBits = GetValidBits(newRow);
        if (newBits.any())
        {
            mPatterns[newBits].insert(id);
        }
    }
}

template<typename P>
template<typename T>
void yaget::comp::Coordinator<P>::RemoveComponent(comp::Id_t id)
{
    auto it = mItems.find(id);
    YAGET_ASSERT(it != mItems.end(), "Item id: '%d' of type: '%s' does not exist in collection.", id, typeid(T).name());
    RemoveComponent(id, std::get<T*>(it->second));
}

template<typename P>
void yaget::comp::Coordinator<P>::RemoveComponents(comp::Id_t id)
{
    auto it = mItems.find(id);
    YAGET_ASSERT(it != mItems.end(), "Item id: '%d' does not exist in collection.", id);

    Row row = it->second;
    meta::for_each(row, [this, id](auto& item)
    {
        if (item)
        {
            RemoveComponent(id, item);
        }
    });

    YAGET_ASSERT(mItems.find(id) == mItems.end(), "Item id: '%d' still exist in collection.", id);
}

template<typename P>
void yaget::comp::Coordinator<P>::RemoveItems(const comp::ItemIds& ids)
{
    for (const auto& id : ids)
    {
        RemoveComponents(id);
    }
}

template<typename P>
typename yaget::comp::Coordinator<P>::Row yaget::comp::Coordinator<P>::FindItem(comp::Id_t id) const
{
    auto it = mItems.find(id);
    if (it != mItems.end())
    {
        return it->second;
    }

    return Row();
}

template<typename P>
template<typename R>
typename R::Row yaget::comp::Coordinator<P>::FindItem(comp::Id_t id) const
{
    if constexpr (std::is_same_v<R::Row, Row>)
    {
        return FindItem(id);
    }
    else
    {
        // TODO: TEST: Verify that all R components are part of P
        //             Verify compile asserts
        typename R::Row requestedComponents;
        constexpr size_t numComponents = std::tuple_size_v<std::remove_reference_t<R::Row>>;
        static_assert(numComponents > 0, "At least one user requested component required");
        static_assert(numComponents <= std::tuple_size_v<std::remove_reference_t<Row>>, "Number of user requested components must be no larger then actual item components.");

        Row itemComponents = FindItem(id);
        internal::RowCopy<numComponents, R::Row, Row>(requestedComponents, itemComponents);

        return requestedComponents;
    }
}

template<typename P>
template<typename R>
typename yaget::comp::ItemIds yaget::comp::Coordinator<P>::GetItemIds() const
{
    std::set<yaget::comp::Id_t> results;

    PatternSet requestBits = MakeBits<typename R::Row>();
    if (requestBits.any())
    {
        for (const auto& it : mPatterns)
        {
            if ((it.first & requestBits) == requestBits)
            {
                results.insert(it.second.begin(), it.second.end());
            }
        }
    }

    return results;
}

template<typename P>
template<typename T>
T* yaget::comp::Coordinator<P>::FindComponent(comp::Id_t id) const
{
    auto row = FindItem(id);
    return std::get<T*>(row);
}

template<typename P>
template<typename T>
yaget::memory::PoolAllocator<T>* yaget::comp::Coordinator<P>::FindAllocator() const
{
    std::any allocator = FindAllocator(std::type_index(typeid(T)));
    if (allocator.has_value())
    {
        try
        {
            std::shared_ptr<memory::PoolAllocator<T>> componentAllocator = std::any_cast<std::shared_ptr<memory::PoolAllocator<T>>>(allocator);
            return componentAllocator.get();
        }
        catch (const std::bad_any_cast& e)
        {
            YAGET_ASSERT(false, "Did not convert any pool allocator: '%s' to: '%s'. %s", allocator.type().name(), typeid(T).name(), e.what());
        }
    }

    return nullptr;
}

template<typename P>
template<typename R>
void yaget::comp::Coordinator<P>::ForEach(const comp::ItemIds& ids, std::function<bool(comp::Id_t id, const typename R::Row& row)> callback)
{
    for (const auto& id : ids)
    {
        auto requestedRow = FindItem<R>(id);
        if (!callback(id, requestedRow))
        {
            break;
        }
    }
}

template<typename P>
template<typename R>
std::size_t yaget::comp::Coordinator<P>::ForEach(std::function<bool(comp::Id_t id, const typename R::Row& row)> callback)
{
    auto ids = GetItemIds<R>();
    ForEach<R>(ids, callback);

    return ids.size();
}
