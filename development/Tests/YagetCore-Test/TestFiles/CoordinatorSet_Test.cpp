#include "pch.h"

#include "YagetCore.h"
#include "Components/CoordinatorSet.h"
#include "Components/CoordinatorSet2.h"
#include "Components/GameSystem.h"
#include "Components/SystemsCoordinator.h"
#include "GameSystem/Messaging.h"
#include "TestHelpers/TestHelpers.h"
#include "Metrics/Concurrency.h"

#include "LoggerCpp/Manager.h"

//CHECK_EQUAL(expected, actual);

class CoordinatorSet : public ::testing::Test
{
protected:
    // void SetUp() override {}
    // void TearDown() override {}

private:
    //yaget::test::Environment mEnvironment;
};


namespace TestObjects
{
    using Messaging = yaget::comp::gs::Messaging<std::shared_ptr<char>>;

    // components for testing
    struct Acomponent { static constexpr int Capacity = 512; size_t mDummy = 0; std::string mText; };
    struct Bcomponent { static constexpr int Capacity = 512; };
    struct Ccomponent { static constexpr int Capacity = 512; };
    struct Dcomponent { static constexpr int Capacity = 512; };
    struct Ecomponent { static constexpr int Capacity = 512; };
    struct Fcomponent { static constexpr int Capacity = 512; };

    struct AglobalCcomponent { };
    struct BglobalCcomponent { };

    // coordinator setup
    using EntityAlias = yaget::comp::RowPolicy<Acomponent*, Bcomponent*, Ccomponent*, Dcomponent*>;
    struct Entity : EntityAlias
    {
        using AutoCleanup = bool;
    };

    using EntityCoordinator = yaget::comp::Coordinator<Entity>;

    using EntityCoordinatorSet = yaget::comp::CoordinatorSet<EntityCoordinator>;

    class ABCD_EntitySystem : public yaget::comp::gs::GameSystem<EntityCoordinatorSet, yaget::comp::gs::NoEndMarker, Messaging, Acomponent*, Bcomponent*, Ccomponent*, Dcomponent*>
    {
    public:
        ABCD_EntitySystem(Messaging& messaging, yaget::Application& app, EntityCoordinatorSet& coordinatorSet)
            : GameSystem("ABCD_EntitySystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
        { }

        int mEntityCounter = 0;

    private:
        void OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, Acomponent* aComponent, Bcomponent* bComponent, Ccomponent* cComponent, Dcomponent* dComponent)
        {
            id;
            gameClock;
            channel;
            aComponent;
            bComponent;
            cComponent;
            dComponent;

            mEntityCounter++;
        }
    };

    class AC_EntitySystem : public yaget::comp::gs::GameSystem<EntityCoordinatorSet, yaget::comp::gs::NoEndMarker, Messaging, Acomponent*, Ccomponent*>
    {
    public:
        AC_EntitySystem(Messaging& messaging, yaget::Application& app, EntityCoordinatorSet& coordinatorSet)
            : GameSystem("AD_EntitySystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
        { }

        int mEntityCounter = 0;

    private:
        void OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, Acomponent* aComponent, Ccomponent* cComponent)
        {
            id;
            gameClock;
            channel;
            aComponent;
            cComponent;

            mEntityCounter++;
        }
    };

    class A_EntitySystem : public yaget::comp::gs::GameSystem<EntityCoordinatorSet, yaget::comp::gs::NoEndMarker, Messaging, Acomponent*>
    {
    public:
        A_EntitySystem(Messaging& messaging, yaget::Application& app, EntityCoordinatorSet& coordinatorSet)
            : GameSystem("A_EntitySystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
        { }

        int mEntityCounter = 0;

    private:
        void OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, Acomponent* aComponent)
        {
            id;
            gameClock;
            channel;
            aComponent;

            mEntityCounter++;
        }
    };

    namespace internal
    {
        using SystemsCoordinatorE = yaget::comp::gs::SystemsCoordinator<EntityCoordinatorSet, Messaging, yaget::Application, ABCD_EntitySystem, AC_EntitySystem, A_EntitySystem>;
    }

    class EntitySystemsCoordinator : public internal::SystemsCoordinatorE
    {
    public:
        EntitySystemsCoordinator(Messaging& m, yaget::Application& app)
            : internal::SystemsCoordinatorE(m, app)
        { }

        ~EntitySystemsCoordinator()
        { }
    };

    using SystemsCoordinatorCapacity = yaget::comp::gs::SystemsCoordinator<EntityCoordinatorSet, Messaging, yaget::Application/*, ABCD_EntitySystem, AC_EntitySystem*/, A_EntitySystem>;


    const int kMaxItems = 10000;
    std::array<TestObjects::Acomponent, kMaxItems> memory{};

    //---------------------------------------------------------------------------------------------------
    // data used in fixing CoordinatorSet from just one and remove hana

    // full item can be composed from A, B, C, D, E and F. Since A, B, C and D are base item, we keep them in one coordinator,
    // where E and F represent buffed status, a much smaller set if items will have that status.
    using BaseEntityAlias = yaget::comp::RowPolicy<Acomponent*, Bcomponent*, Ccomponent*, Dcomponent*>;
    struct BaseEntity : BaseEntityAlias { using AutoCleanup = bool; };

    using BaseEntityCoordinator = yaget::comp::Coordinator<BaseEntity>;

    using BuffedEntityAlias = yaget::comp::RowPolicy<Ecomponent*, Fcomponent*>;
    struct BuffedEntity : BuffedEntityAlias { using AutoCleanup = bool; };

    using BuffedEntityCoordinator = yaget::comp::Coordinator<BuffedEntity>;

    using KnightEntityCoordinatorSet = yaget::comp::CoordinatorSet2<BaseEntityCoordinator, BuffedEntityCoordinator>;



} // namespace TestObjects


#include <tuple>

TEST_F(CoordinatorSet, CoordinatorSet2)
{
    using namespace yaget;

    TestObjects::KnightEntityCoordinatorSet knightEntities{};

    using RequestedRow = std::tuple<TestObjects::Bcomponent*, TestObjects::Ccomponent*, TestObjects::Dcomponent*, TestObjects::Ecomponent*>;

    const auto numProcessed = knightEntities.ForEach<RequestedRow>([](comp::Id_t id, auto components)
    {
        return 0;
    });


}


TEST_F(CoordinatorSet, ComponentAccess)
{
    using namespace yaget;

    test::ApplicationFramework<TestObjects::Messaging, TestObjects::EntitySystemsCoordinator> testerFramework("ComponentAccess");

    auto& idGameCache = testerFramework.Ids();
    auto& entitySystemsCoordinator = testerFramework.SystemsCoordinator();
    auto& entityCoordinator = entitySystemsCoordinator.GetCoordinator<TestObjects::Entity>();
     
    // item #1
    comp::Id_t itemId = idspace::get_burnable(idGameCache);
    entityCoordinator.AddComponent<TestObjects::Acomponent>(itemId);
    entityCoordinator.AddComponent<TestObjects::Bcomponent>(itemId);
    entityCoordinator.AddComponent<TestObjects::Ccomponent>(itemId);
    entityCoordinator.AddComponent<TestObjects::Dcomponent>(itemId);

    // item #2
    itemId = idspace::get_burnable(idGameCache);
    entityCoordinator.AddComponent<TestObjects::Acomponent>(itemId);
    entityCoordinator.AddComponent<TestObjects::Bcomponent>(itemId);
    entityCoordinator.AddComponent<TestObjects::Ccomponent>(itemId);
    entityCoordinator.AddComponent<TestObjects::Dcomponent>(itemId);

    // item #3
    itemId = idspace::get_burnable(idGameCache);
    entityCoordinator.AddComponent<TestObjects::Acomponent>(itemId);

    // item #4
    itemId = idspace::get_burnable(idGameCache);
    entityCoordinator.AddComponent<TestObjects::Bcomponent>(itemId);

    // item #5
    itemId = idspace::get_burnable(idGameCache);
    entityCoordinator.AddComponent<TestObjects::Ccomponent>(itemId);

    // item #6
    itemId = idspace::get_burnable(idGameCache);
    entityCoordinator.AddComponent<TestObjects::Dcomponent>(itemId);

    // item #7
    itemId = idspace::get_burnable(idGameCache);
    entityCoordinator.AddComponent<TestObjects::Acomponent>(itemId);
    entityCoordinator.AddComponent<TestObjects::Dcomponent>(itemId);

    // item #8
    itemId = idspace::get_burnable(idGameCache);
    entityCoordinator.AddComponent<TestObjects::Bcomponent>(itemId);
    entityCoordinator.AddComponent<TestObjects::Ccomponent>(itemId);

    // item #9
    itemId = idspace::get_burnable(idGameCache);
    entityCoordinator.AddComponent<TestObjects::Acomponent>(itemId);
    entityCoordinator.AddComponent<TestObjects::Bcomponent>(itemId);
    entityCoordinator.AddComponent<TestObjects::Ccomponent>(itemId);

    // item #10
    itemId = idspace::get_burnable(idGameCache);
    entityCoordinator.AddComponent<TestObjects::Bcomponent>(itemId);
    entityCoordinator.AddComponent<TestObjects::Ccomponent>(itemId);
    entityCoordinator.AddComponent<TestObjects::Dcomponent>(itemId);

    // item #11
    itemId = idspace::get_burnable(idGameCache);
    entityCoordinator.AddComponent<TestObjects::Acomponent>(itemId);
    entityCoordinator.AddComponent<TestObjects::Bcomponent>(itemId);

    // item #12
    itemId = idspace::get_burnable(idGameCache);
    entityCoordinator.AddComponent<TestObjects::Acomponent>(itemId);
    entityCoordinator.AddComponent<TestObjects::Ccomponent>(itemId);

    time::GameClock gameClock;
    metrics::Channel channel("Test", YAGET_METRICS_CHANNEL_FILE_LINE);

    entitySystemsCoordinator.Tick(gameClock, channel);

    const auto& systemABCD = entitySystemsCoordinator.GetSystem<TestObjects::ABCD_EntitySystem>();
    EXPECT_EQ(2, systemABCD.mEntityCounter);

    const auto& systemAC = entitySystemsCoordinator.GetSystem<TestObjects::AC_EntitySystem>();
    EXPECT_EQ(4, systemAC.mEntityCounter);

    const auto& systemA = entitySystemsCoordinator.GetSystem<TestObjects::A_EntitySystem>();
    EXPECT_EQ(7, systemA.mEntityCounter);
}


TEST_F(CoordinatorSet, ComponentCapacity)
{
    using namespace yaget;

    ylog::Manager::AddOverrideFilter(LOG_TAG("TEST"));

    test::ApplicationFramework<TestObjects::Messaging, TestObjects::SystemsCoordinatorCapacity> testerFramework("ComponentCapacity");

    auto& entitySystemsCoordinator = testerFramework.SystemsCoordinator();

    const auto ScoperUnit = time::kMicrosecondUnit;
    {
        metrics::Channel channel("ComponentCapacity.Adding", YAGET_METRICS_CHANNEL_FILE_LINE);

        auto& idGameCache = testerFramework.Ids();
        auto& entityCoordinator = entitySystemsCoordinator.GetCoordinator<TestObjects::Entity>();

        const auto& message = fmt::format("Creating '{}' entities", conv::ToThousandsSep(TestObjects::kMaxItems));
        metrics::TimeScoper<ScoperUnit> intTimer("TEST", message.c_str(), YAGET_LOG_FILE_LINE_FUNCTION);

        for (auto i = 0; i < TestObjects::kMaxItems; ++i)
        {
            comp::Id_t itemId = idspace::get_burnable(idGameCache);

            entityCoordinator.AddComponent<TestObjects::Acomponent>(itemId);
            entityCoordinator.AddComponent<TestObjects::Bcomponent>(itemId);
            entityCoordinator.AddComponent<TestObjects::Ccomponent>(itemId);
            entityCoordinator.AddComponent<TestObjects::Dcomponent>(itemId);
        }
    }

    const int kNumTicks = 20;
    {
        int accumulator = 0;
        const time::Microsecond_t kFixedDeltaTime = time::kDeltaTime_60;

        time::GameClock gameClock;
        metrics::Channel channel("ComponentCapacity.Tick", YAGET_METRICS_CHANNEL_FILE_LINE);

        for (int i = 0; i < kNumTicks; ++i)
        {
            const auto& message = fmt::format("Tick {}: Ticking '{}' entities", i, conv::ToThousandsSep(TestObjects::kMaxItems));
            metrics::TimeScoper<ScoperUnit> intTimer("TEST", message.c_str(), YAGET_LOG_FILE_LINE_FUNCTION);
            intTimer.SetAccumulator(&accumulator);

            entitySystemsCoordinator.Tick(gameClock, channel);

            gameClock.Tick(kFixedDeltaTime);
        }

        float avgTick = (accumulator * 1.0f) / kNumTicks;
        YLOG_NOTICE("TEST", "Average Tick %s (%s) after %d tries.", conv::ToThousandsSep(avgTick).c_str(), metrics::UnitName(ScoperUnit).c_str(), kNumTicks);
    }

    size_t counter = 0;
    {
        auto& entityCoordinator = entitySystemsCoordinator.GetCoordinator<TestObjects::Entity>();
        auto& allocator = entityCoordinator.GetAllocator<TestObjects::Acomponent>();

        metrics::Channel channel("ComponentCapacity.Iterator", YAGET_METRICS_CHANNEL_FILE_LINE);

        for (int i = 0; i < kNumTicks; ++i)
        {
            metrics::Channel channel(fmt::format("Iterator pass '{}'", i), YAGET_METRICS_CHANNEL_FILE_LINE);

            for (const auto& it : allocator)
            {
                //const auto& message = fmt::format("Processing '{}' entity", it.mDummy);
                //metrics::Channel systemChannel(message, YAGET_METRICS_CHANNEL_FILE_LINE);

                counter += it.mDummy;
            }
        }
    }

    {
        //std::array<TestObjects::Acomponent, kMaxItems> memory{};

        for (size_t i = 0; i < TestObjects::kMaxItems; ++i)
        {
            TestObjects::memory[i] = { i, "" };
        }

        metrics::Channel channel("ComponentCapacity.RawArray", YAGET_METRICS_CHANNEL_FILE_LINE);

        for (int t = 0; t < kNumTicks; ++t)
        {
            metrics::Channel channelTick(fmt::format("For loop pass '{}'", t), YAGET_METRICS_CHANNEL_FILE_LINE);

            for (size_t i = 0; i < TestObjects::kMaxItems; ++i)
            {
                const auto& element = TestObjects::memory[i];

                counter += element.mDummy;

                //metrics::Channel channelFor(fmt::format("Processing '{}' element", element.mDummy), YAGET_METRICS_CHANNEL_FILE_LINE);
            }
        }
    }

    YLOG_NOTICE("TEST", "Counter is: '%d'", counter);
}