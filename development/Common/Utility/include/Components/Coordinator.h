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


namespace yaget::comp
{
    namespace internal
    {
        // compile time switch to run auto cleanup on left over items during destruction
        template<typename T>
        concept has_auto_cleanup = requires { typename T::AutoCleanup; };


        // Return tuple of Allocators for each component
        template <std::size_t TupleIndex, std::size_t MaxTupleSize, typename Tuple>
        constexpr auto coordinator_allocator_combine()
        {
            using ComponentType = typename std::tuple_element<TupleIndex, Tuple>::type;

            using CompType = typename meta::strip_qualifiers_t<ComponentType>;
            std::tuple<memory::PoolAllocator<CompType>> currentRow;

            if constexpr (TupleIndex + 1 < MaxTupleSize)
            {
                auto nextRow = std::move(coordinator_allocator_combine<TupleIndex + 1, MaxTupleSize, Tuple>());
                return std::tuple_cat(std::move(currentRow), std::move(nextRow));

            }
            else
            {
                return currentRow;
            }
        }

    }

    template <typename Tuple>
    struct coordinator_allocator_combine
    {
        using type = decltype(internal::coordinator_allocator_combine<0, std::tuple_size_v<std::remove_reference_t<Tuple>>, Tuple>());
    };

    template<typename Tuple>
    using coordinator_allocator_combine_t = typename coordinator_allocator_combine<Tuple>::type;


    // Coordinator stores map of items (keyed on item id), manages creation, storage and deletion of components.
    // It uses PoolAllocator as a storage for components.
    template <typename P>
    class Coordinator : public Noncopyable<Coordinator<P>>
    {
    public:
        using Policy = P;
        using FullRow = typename Policy::Row;

        static constexpr size_t NumComponents = Policy::NumComponents;

        using PatternSet = std::bitset<NumComponents>;
        using Allocators = coordinator_allocator_combine_t<FullRow>;

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
        FullRow FindItem(comp::Id_t id) const;

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

    private:
        // Helper method to find a specific component allocator
        template<typename T>
        memory::PoolAllocator<T>& FindAllocator() const;

        template<typename T>
        constexpr meta::bits_t MakeBit() const
        {
            // You can use this pattern before passing T: using CompType = typename meta::strip_qualifiers_t<T>;
            static_assert(std::is_pointer_v<T> == false, "template T must not be a pointer");
            return static_cast<meta::bits_t>(1) << meta::Index<T*, FullRow>::value;
        }

        template<typename... Args>
        constexpr meta::bits_t GetValidBits(const std::tuple<Args...>& item) const
        {
            meta::bits_t bits = 0;

            meta::for_each(item, [this, &bits]<typename T0>(T0& compType)
            {
                if (compType)
                {
                    using CompType = typename meta::strip_qualifiers_t<T0>;

                    bits = bits | MakeBit<CompType>();
                }
            });

            return bits;
        }

        Allocators mAllocators;

        // Actual item created from Allocators
        std::map<comp::Id_t, FullRow> mItems;

        // map from unique bits to all id's which contain that specific set of components
        using Patterns = std::unordered_map<PatternSet, std::set<comp::Id_t>>;
        Patterns mPatterns;

        const Strings mComponentNames = comp::db::GetPolicyRowNames<P::Row>();
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
} // namespace yaget

template<typename P>
yaget::comp::Coordinator<P>::~Coordinator()
{
    if constexpr (internal::has_auto_cleanup<Policy>)
    {
        YLOG_CWARNING("GSYS", mItems.empty(), "Coordinator [%s] still has outstanding '%d' component(s): [%s], cleaning up...",
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
template<typename T, typename... Args>
T* yaget::comp::Coordinator<P>::AddComponent(comp::Id_t id, Args&&... args)
{
    FullRow row = FindItem(id);
    meta::bits_t currentBits = GetValidBits(row);
    const meta::bits_t newBit = MakeBit<T>();
    YAGET_ASSERT((currentBits & newBit) != newBit, "Reqested new component of type: '%s' for Item: '%d' already exist in Coordinator.", typeid(T).name(), id);

    if (currentBits)
    {
        mPatterns[currentBits].erase(id);
        if (mPatterns[currentBits].empty())
        {
            mPatterns.erase(currentBits);
        }
    }

    memory::PoolAllocator<T>& componentAllocator = FindAllocator<T>();
    T* newComponenet = componentAllocator.Allocate(std::forward<comp::Id_t>(id), std::forward<Args>(args)...);

    std::get<T*>(mItems[id]) = newComponenet;

    FullRow newRow = FindItem(id);
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

    FullRow row = FindItem(id);
    PatternSet currentBits = GetValidBits(row);
    mPatterns[currentBits].erase(id);

    memory::PoolAllocator<T>& componentAllocator = FindAllocator<T>();
    componentAllocator.Free(component);
    std::get<T*>(mItems[id]) = nullptr;
    component = nullptr;

    if (mItems[id] == FullRow{})
    {
        mItems.erase(id);
        mPatterns.erase(currentBits);
    }
    else
    {
        FullRow newRow = FindItem(id);
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

    FullRow row = it->second;
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
typename yaget::comp::Coordinator<P>::FullRow yaget::comp::Coordinator<P>::FindItem(comp::Id_t id) const
{
    auto it = mItems.find(id);
    if (it != mItems.end())
    {
        return it->second;
    }

    return FullRow{};
}

template<typename P>
template<typename R>
typename R::Row yaget::comp::Coordinator<P>::FindItem(comp::Id_t id) const
{
    if constexpr (std::is_same_v<R::Row, FullRow>)
    {
        return FindItem(id);
    }
    else
    {
        // TODO: TEST: Verify that all R components are part of P
        typename R::Row requestedComponents;
        constexpr size_t numComponents = std::tuple_size_v<std::remove_reference_t<typename R::Row>>;
        static_assert(numComponents > 0, "At least one user requested component required");
        static_assert(numComponents <= std::tuple_size_v<std::remove_reference_t<FullRow>>, "Number of user requested components must be no larger then actual item components.");

        FullRow itemComponents = FindItem(id);
        internal::RowCopy<numComponents, typename R::Row, FullRow>(requestedComponents, itemComponents);

        return requestedComponents;
    }
}

template<typename P>
template<typename R>
typename yaget::comp::ItemIds yaget::comp::Coordinator<P>::GetItemIds() const
{
    std::set<yaget::comp::Id_t> results;

    PatternSet requestBits = meta::tuple_bit_pattern_v<FullRow, typename R::Row>;
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
yaget::memory::PoolAllocator<T>& yaget::comp::Coordinator<P>::FindAllocator() const
{
    using PA = memory::PoolAllocator<T>;
    return const_cast<PA&>(std::get<PA>(mAllocators));
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
