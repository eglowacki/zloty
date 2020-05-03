#include "UnitTest++.h"
#include "Components/Coordinator.h"
#include "Components/LocationComponent.h"
#include "Components/PhysicsComponent.h"
#include "Components/GameSystem.h"
#include "Components/GameCoordinator.h"
#include "IdGameCache.h"
#include "Fmt/format.h"
#include "Metrics/Gather.h"
#include "Metrics/Concurrency.h"
#include <tuple>


//CHECK_EQUAL(expected, actual);

class DummyComp : yaget::Noncopyable<DummyComp>
{
public:
    static constexpr int Capacity = 4;
};

class DummyComp2 : yaget::Noncopyable<DummyComp2>
{
public:
    static constexpr int Capacity = 4;
};


template <typename T, typename Tuple>
struct has_type;

template <typename T, typename... Us>
struct has_type<T, std::tuple<Us...>> : std::disjunction<std::is_same<T, Us>...> {};

TEST(CoordinatorMulti)
{
    using namespace yaget;

    using Entity = comp::RowPolicy<comp::LocationComponent*, comp::PhysicsComponent*>;
    using GlobalEntity = comp::RowPolicy<comp::PhysicsWorldComponent*, DummyComp*, DummyComp2*>;

    using GamePolicy = comp::CoordinatorPolicy<Entity, GlobalEntity>;

    using PhysicsSystem = comp::GameSystem<comp::gs::EndMarkerNo<0>, comp::PhysicsWorldComponent*>;
    using LocationGather = comp::GameSystem<comp::gs::EndMarkerYes<1>, comp::LocationComponent*>;

    using namespace std::placeholders;
    PhysicsSystem physWorldGameSystem("PhysicsSystem", [](comp::Id_t /*id*/, const time::GameClock& gameClock, metrics::Channel& channel, comp::PhysicsWorldComponent* physWorld)
    {
        physWorld->Update(gameClock, channel);
    });

    LocationGather locationGameSystem("LocationGather", [](comp::Id_t /*id*/, const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/, comp::LocationComponent* /*locationComponent*/)
    {
    });

    using GameCoordinator = GameCoordinator<GamePolicy, PhysicsSystem*, LocationGather*>;
    metrics::Channel channel("Test.Dummy", YAGET_METRICS_CHANNEL_FILE_LINE);
    time::GameClock gameClock;
    GameCoordinator gameCoordinator(&physWorldGameSystem, &locationGameSystem);

    gameCoordinator.GameUpdate(gameClock, channel);
}


TEST(CoordinatorHashBits)
{
    using namespace yaget;

    using Entity = comp::RowPolicy<comp::LocationComponent*, comp::PhysicsComponent*, comp::PhysicsWorldComponent*, DummyComp*, DummyComp2*>;
    using EntityCoordinator = comp::Coordinator<Entity>;

    EntityCoordinator coordinator;
    IdGameCache idGameCache(nullptr);

    const comp::Id_t itemIdA = idspace::get_burnable(idGameCache);

    comp::PhysicsWorldComponent* physicsWorld = coordinator.AddCompponent<comp::PhysicsWorldComponent>(itemIdA);
    comp::LocationComponent* locationComponent = coordinator.AddCompponent<comp::LocationComponent>(itemIdA);
    coordinator.AddCompponent<comp::PhysicsComponent>(itemIdA, physicsWorld, physics::BoxCollisionShape(math3d::Vector3(10, 10, 0.5f), 0.0f), locationComponent);
    coordinator.AddCompponent<DummyComp>(itemIdA);

    const comp::Id_t itemIdB = idspace::get_burnable(idGameCache);

    coordinator.AddCompponent<comp::LocationComponent>(itemIdB);
    coordinator.AddCompponent<DummyComp>(itemIdB);

    const comp::Id_t itemIdC = idspace::get_burnable(idGameCache);

    coordinator.AddCompponent<DummyComp>(itemIdC);

    using FullEntity = comp::RowPolicy<comp::PhysicsWorldComponent*, comp::LocationComponent*, comp::PhysicsComponent*, DummyComp*>;
    auto itemListA = coordinator.GetItemIds<FullEntity>();
    CHECK_EQUAL(itemIdA, *itemListA.begin());

    using LocationEntity = comp::RowPolicy<comp::LocationComponent*, DummyComp*>;
    auto itemListB = coordinator.GetItemIds<LocationEntity>();
    CHECK_EQUAL(2, itemListB.size());
    CHECK(itemListB.find(itemIdA) != itemListB.end());
    CHECK(itemListB.find(itemIdB) != itemListB.end());

    int counter = 0;
    coordinator.ForEach<LocationEntity>([&counter](comp::Id_t /*id*/, const auto& /*row*/)
    {
        ++counter;
        return true;
    });
    CHECK_EQUAL(itemListB.size(), counter);

    using DummyEntity = comp::RowPolicy<DummyComp*>;
    auto itemListC = coordinator.GetItemIds<DummyEntity>();
    CHECK_EQUAL(3, itemListC.size());
    CHECK(itemListC.find(itemIdA) != itemListC.end());
    CHECK(itemListC.find(itemIdB) != itemListC.end());
    CHECK(itemListC.find(itemIdC) != itemListC.end());

    counter = 0;
    coordinator.ForEach<DummyEntity>(itemListC, [&counter](yaget::comp::Id_t /*id*/, const auto& /*row*/)
    {
        ++counter;
        return true;
    });
    CHECK_EQUAL(itemListC.size(), counter);

    using WrongEntity = comp::RowPolicy<DummyComp2*, DummyComp*>;
    auto itemListD = coordinator.GetItemIds<WrongEntity>();
    CHECK_EQUAL(0, itemListD.size());

    coordinator.RemoveCompponents(itemIdA);
    coordinator.RemoveCompponents(itemIdB);
    coordinator.RemoveCompponents(itemIdC);
}


TEST(CoordinatorSystem)
{
    using namespace yaget;

    //-------------------------------------------------------------------------------------------------------------
    // test that Coordinator requires at least one component (uncomment this one and comment one below
    //using GlobalEntity = comp::RowPolicy<>;
    using GlobalEntity = comp::RowPolicy<comp::PhysicsWorldComponent*, DummyComp*>;
    //-------------------------------------------------------------------------------------------------------------
    using Entity = comp::RowPolicy<comp::LocationComponent*, comp::PhysicsComponent*>;

    using GlobalEntityCoordinator = comp::Coordinator<GlobalEntity>;
    using EntityCoordinator = comp::Coordinator<Entity>;

    GlobalEntityCoordinator globalCoordinator;
    EntityCoordinator coordinator;
    IdGameCache idGameCache(nullptr);

    const comp::Id_t kPhysicsWorldId = idspace::get_burnable(idGameCache);
    comp::PhysicsWorldComponent* physicsWorld = globalCoordinator.AddCompponent<comp::PhysicsWorldComponent>(kPhysicsWorldId);
    physicsWorld;

    /*DummyComp* dummyComp = */globalCoordinator.AddCompponent<DummyComp>(kPhysicsWorldId);

    using TestEntity = comp::RowPolicy<comp::LocationComponent*, comp::PhysicsComponent*>;

    const comp::Id_t kTestId = idspace::get_burnable(idGameCache);
    comp::LocationComponent* locComp = coordinator.AddCompponent<comp::LocationComponent>(kTestId);
    coordinator.AddCompponent<comp::PhysicsComponent>(kTestId, physicsWorld, physics::BoxCollisionShape(math3d::Vector3(10, 10, 0.5f), 0.0f), locComp);

    coordinator.RemoveCompponents(kTestId);
    globalCoordinator.RemoveCompponents(kPhysicsWorldId);
}

TEST(Coordinator_Hash)
{
    using namespace yaget;

    const comp::Id_t itemId_10 = 10;

    using ItemRow = comp::RowPolicy<comp::LocationComponent*>;
    using ItemCoordinator = comp::Coordinator<ItemRow>;

    ItemCoordinator coordinator;

    comp::LocationComponent* locationComponent = coordinator.AddCompponent<comp::LocationComponent>(itemId_10, math3d::Vector3(1, 2, 3), math3d::Quaternion(1, 2, 3, 1));
    uint64_t hashStateA = locationComponent->GetStateHash();
    uint64_t hashStateB = locationComponent->GetStateHash();
    CHECK_EQUAL(hashStateB, hashStateA);

    locationComponent->SetPosition(math3d::Vector3(4, 5, 6));
    uint64_t hashStateC = locationComponent->GetStateHash();
    CHECK(hashStateC != hashStateA);
    CHECK_EQUAL(hashStateC, locationComponent->GetStateHash());

    locationComponent->SetScale(math3d::Vector3(7, 8, 9));
    uint64_t hashStateD = locationComponent->GetStateHash();
    CHECK(hashStateC != hashStateD);

    locationComponent->SetScale(math3d::Vector3(7, 8, 9));
    CHECK_EQUAL(hashStateD, locationComponent->GetStateHash());

    coordinator.RemoveCompponents(itemId_10);
}


TEST(Coordinator)
{
    using namespace yaget;

    using ItemRow = comp::RowPolicy<comp::LocationComponent*, comp::PhysicsComponent*, comp::PhysicsWorldComponent*>;
    using ItemCoordinator = comp::Coordinator<ItemRow>;
    ItemCoordinator coordinator;

    const comp::Id_t itemId_10 = 10;
    const comp::Id_t itemId_20 = 20;
    const comp::Id_t noItemId = 500;

    // create item with couple components, delete them one by one
    comp::PhysicsWorldComponent* physicsWorld = coordinator.AddCompponent<comp::PhysicsWorldComponent>(itemId_10);
    CHECK(physicsWorld);
    CHECK_EQUAL(physicsWorld->Id(), itemId_10);

    comp::LocationComponent* locationComponent = coordinator.AddCompponent<comp::LocationComponent>(itemId_10, math3d::Vector3(1, 2, 3), math3d::Quaternion(1, 2, 3, 1));

    CHECK(locationComponent);
    CHECK_EQUAL(locationComponent->Id(), itemId_10);
    CHECK(locationComponent->GetPosition() == math3d::Vector3(1, 2, 3));
    CHECK(locationComponent->GetOrientation() == math3d::Quaternion(1, 2, 3, 1));

    CHECK_EQUAL(coordinator.FindComponent<comp::PhysicsWorldComponent>(itemId_10), physicsWorld);
    CHECK_EQUAL(coordinator.FindComponent<comp::LocationComponent>(itemId_10), locationComponent);
    CHECK(coordinator.FindItem(itemId_10) == ItemCoordinator::Row(locationComponent, nullptr, physicsWorld));

    // create item with couple components, delete them all at once
    comp::LocationComponent* locationComponent_20 = coordinator.AddCompponent<comp::LocationComponent>(itemId_20);
    CHECK(locationComponent_20);
    CHECK_EQUAL(locationComponent_20->Id(), itemId_20);
    CHECK(locationComponent_20->Matrix().Translation() == math3d::Matrix::Identity.Translation());

    const math3d::Vector3 kCheckLocation(1, 2, 3);
    comp::PhysicsComponent::Params params = comp::physics::CreateInitState(comp::physics::BoxShape, math3d::Vector3(10, 10, 0.5f));
    math3d::Matrix matrixPlaceholder = math3d::Matrix::CreateTranslation(kCheckLocation);
    params.matrix = &matrixPlaceholder;
    comp::PhysicsComponent* physicsComponent = coordinator.AddCompponent<comp::PhysicsComponent>(itemId_20, physicsWorld, params);
    CHECK(physicsComponent);
    CHECK_EQUAL(physicsComponent->Id(), itemId_20);
    CHECK(coordinator.FindItem(itemId_20) == ItemCoordinator::Row(locationComponent_20, physicsComponent, nullptr));
    CHECK(coordinator.FindItem(noItemId) == ItemCoordinator::Row());

    // connect Location and Physics for update
    bool result = physicsComponent->ConnectTrigger(comp::PhysicsComponent::TransformChanged, [locationComponent](const comp::Component& from)
    {
        const comp::PhysicsComponent& physicsComponent = static_cast<const comp::PhysicsComponent&>(from);
        locationComponent->SetPosition(physicsComponent.GetPosition());
        locationComponent->SetOrientation(physicsComponent.GetOrientation());
    });
    CHECK(result);
    CHECK(locationComponent->GetPosition() == kCheckLocation);

    // check find item returning different layout for a row
    using Tree = comp::RowPolicy<comp::LocationComponent*, comp::PhysicsWorldComponent*>;
    Tree::Row tree = coordinator.FindItem<Tree>(itemId_20);
    CHECK_EQUAL(locationComponent_20, std::get<comp::LocationComponent*>(tree));
    CHECK_EQUAL(nullptr, std::get<comp::PhysicsWorldComponent*>(tree));

    // verify game system class
    int counter = 0;
    comp::GameSystem<comp::gs::EndMarkerNo<0>, comp::PhysicsWorldComponent*> gameSystem("PhysicsWorldSystem", [&counter](comp::Id_t /*id*/, const time::GameClock& gameClock, metrics::Channel& channel, yaget::comp::PhysicsWorldComponent* physicsWorld)
    {
        counter++;
        physicsWorld->Update(gameClock, channel);
    });

    using PhysicsItem = comp::RowPolicy<comp::PhysicsWorldComponent*>;
    PhysicsItem::Row physicsItem = coordinator.FindItem<PhysicsItem>(itemId_10);
    CHECK_EQUAL(std::get<comp::PhysicsWorldComponent*>(physicsItem), physicsWorld);

    // timing results for creation of some large number simple components
    using TreeItem = comp::RowPolicy<comp::LocationComponent*>;
    TreeItem::Row tree_10 = coordinator.FindItem<TreeItem>(itemId_10);
    const int kNumComponents = 1000;

    {
        memory::PoolAllocator<comp::LocationComponent, 64> lcAllocator;
        std::vector<comp::LocationComponent*> pointerList;
        pointerList.reserve(kNumComponents);

        {
            std::string message = fmt::format("Allocate {} Components", kNumComponents);
            metrics::TimeScoper<time::kMicrosecondUnit> timeScoper(message.c_str(), YAGET_LOG_FILE_LINE_FUNCTION);
            for (int i = 0; i < kNumComponents; ++i)
            {
                comp::LocationComponent* testClass = lcAllocator.Allocate(i);
                pointerList.push_back(testClass);
            }
        }

        {
            std::string message = fmt::format("Free {} Components", kNumComponents);
            metrics::TimeScoper<time::kMicrosecondUnit> timeScoper(message.c_str(), YAGET_LOG_FILE_LINE_FUNCTION);
            std::for_each(pointerList.begin(), pointerList.end(), [&lcAllocator](comp::LocationComponent* element)
            {
                lcAllocator.Free(element);
            });
        }
    }

    {
        std::string message = fmt::format("Add {} Components", kNumComponents);
        metrics::TimeScoper<time::kMicrosecondUnit> timeScoper(message.c_str(), YAGET_LOG_FILE_LINE_FUNCTION);
        for (int i = 0; i < kNumComponents; ++i)
        {
            coordinator.AddCompponent<comp::LocationComponent>(i + 1000, math3d::Vector3(1, 2, 3), math3d::Quaternion(1, 2, 3, 1));
        }
    }

    // clear one at the time for id 20
    coordinator.RemoveCompponent(physicsComponent->Id(), physicsComponent);
    CHECK(physicsComponent == nullptr);
    CHECK(coordinator.FindItem(itemId_20) == ItemCoordinator::Row(locationComponent_20, nullptr, nullptr));

    coordinator.RemoveCompponent(locationComponent_20->Id(), locationComponent_20);
    CHECK(locationComponent_20 == nullptr);
    CHECK(coordinator.FindItem(itemId_20) == ItemCoordinator::Row());

    // clear all at one for item 10
    coordinator.RemoveCompponents(itemId_10);
    CHECK(coordinator.FindItem(itemId_10) == ItemCoordinator::Row());

    {
        std::string message = fmt::format("Remove {} Components", kNumComponents);
        metrics::TimeScoper<time::kMicrosecondUnit> timeScoper(message.c_str(), YAGET_LOG_FILE_LINE_FUNCTION);
        for (int i = 0; i < kNumComponents; ++i)
        {
            coordinator.RemoveCompponent<comp::LocationComponent>(i + 1000);
        }
    }

    int z = 0;
    z;
}
