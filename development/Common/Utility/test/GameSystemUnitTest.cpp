#include "UnitTest++.h"
#include "Components/PayloadStager.h"
#include "Components/GameSystem.h"
#include "Components/Coordinator.h"
#include "Components/Component.h"
#include "Components/LocationComponent.h"
#include "Components/PhysicsComponent.h"
#include "Components/Collectors.h"
#include "Metrics/Concurrency.h"
#include "IdGameCache.h"
#include "ThreadModel/JobPool.h"
#include <memory>


namespace yaget
{
} // namespace yaget

namespace
{

    class DummyComp : yaget::Noncopyable<DummyComp>
    {
    public:
        static constexpr int Capacity = 4;

        DummyComp(yaget::comp::Id_t id) : mId(id) {}
        yaget::comp::Id_t mId;
    };

    class DummyComp2 : yaget::Noncopyable<DummyComp2>
    {
    public:
        static constexpr int Capacity = 4;

        DummyComp2(yaget::comp::Id_t id) : mId(id) {}
        yaget::comp::Id_t mId;
    };

    using SceneChunkCollector = yaget::comp::CollectorHelper<yaget::comp::SceneChunk>;

    class RenderCollector : public SceneChunkCollector
    {
    public:
        RenderCollector() : SceneChunkCollector()
        {}

        void Process(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& /*channel*/, yaget::comp::LocationComponent* location)
        {
            if (IsEndMarker(id, gameClock))
            {
                return;
            }

            size_t currentHash = location->GetStateHash();
            if (UpdateHash(id, std::type_index(typeid(yaget::comp::LocationComponent)), currentHash))
            {
                mPayload->mLocations.push_back(yaget::comp::LocationChunk{ id, location->GetPosition(), location->GetOrientation() });
            }
            else
            {
                mPayload->mActiveIds.insert(id);
            }
        }
    };


} // namespace


//CHECK_EQUAL(expected, actual);

TEST(PayloadStager)
{
    using namespace yaget;

    comp::PayloadStager<comp::SceneChunk> stager;

    mt::JobPool jobPool("StagerTest", 2, mt::JobPool::Behaviour::StartAsPause);
    std::atomic_bool quit{ false };
    int payloadsCreated = 0;

    jobPool.AddTask([&stager, &quit, &payloadsCreated]()
    {
        int counter = 0;
        while (!quit)
        {
            auto payload = stager.CreatePayload();
            payload->mActiveIds.insert(++counter);

            payloadsCreated++;
            stager.SetPayload(payload);

            std::this_thread::yield();
        }
    });

    int payloadsConsumed = 0;
    jobPool.AddTask([&stager, &quit, &payloadsConsumed]()
    {
        while (!quit)
        {
            if (const auto& payload = stager.ConsumePayload())
            {
                payloadsConsumed++;
                CHECK_EQUAL(1, payload->mActiveIds.size());
            }

            std::this_thread::yield();
        }
    });

    jobPool.UnpauseAll();
    platform::Sleep(100, time::kMilisecondUnit);
    quit = true;

    YLOG_INFO("PROF", "Payloads Created: '%d', Comsumed: '%d', Dropped: '%d'.", payloadsCreated, payloadsConsumed, payloadsCreated - payloadsConsumed);
}


TEST(GameSystem)
{
    using namespace yaget;

    using Entity = comp::RowPolicy<comp::LocationComponent*, comp::PhysicsComponent*, comp::PhysicsWorldComponent*, DummyComp*, DummyComp2*>;
    using EntityCoordinator = comp::Coordinator<Entity>;

    EntityCoordinator coordinator;
    IdGameCache idGameCache(nullptr);
    const int kMaxItems = 10;

    comp::Id_t physWorldId = idspace::get_burnable(idGameCache);
    comp::PhysicsWorldComponent* physicsWorld = coordinator.AddComponent<comp::PhysicsWorldComponent>(physWorldId);

    comp::ItemIds ids;

    for (int i = 0; i < kMaxItems; ++i)
    {
        comp::Id_t nextId = idspace::get_burnable(idGameCache);
        ids.insert(nextId);

        comp::LocationComponent* locationComponent = coordinator.AddComponent<comp::LocationComponent>(nextId);
        coordinator.AddComponent<comp::PhysicsComponent>(nextId, physicsWorld, physics::BoxCollisionShape(math3d::Vector3(10, 10, 0.5f), 0.0f), locationComponent);
        coordinator.AddComponent<DummyComp>(nextId);
        coordinator.AddComponent<DummyComp2>(nextId);
    }

    RenderCollector renderCollector;          // collect all data from logic thread and fill in stager
    time::GameClock gameClock;

    using namespace std::placeholders;
    comp::GameSystem<comp::gs::EndMarkerYes, comp::LocationComponent*> locationGameSystem("LocationSystem", [&renderCollector](auto&&... params) { renderCollector.Process(params...); });
    comp::GameSystem<comp::gs::EndMarkerNo, comp::PhysicsWorldComponent*> physWorldGameSystem("PhysicsWorldSystem", [](comp::Id_t /*id*/, const time::GameClock& gameClock, metrics::Channel& channel, comp::PhysicsWorldComponent* physWorld)
    {
        physWorld->Update(gameClock, channel);
    });

    metrics::Channel channel("DummyChannel");

    physWorldGameSystem.Update(gameClock, channel, coordinator);
    locationGameSystem.Update(gameClock, channel, coordinator);      // run over all entities with specified components (extract from coordinator). 
                                                            // This in turn will call RenderCollector::Process for each enity

    comp::PayloadStager<comp::SceneChunk>::ConstPayload payload = renderCollector.PayloadStager().GetPayload();
    CHECK_EQUAL(kMaxItems, payload->mLocations.size());
    CHECK_EQUAL(0, payload->mActiveIds.size());

    gameClock.Tick(time::kDeltaTime_60);
    physWorldGameSystem.Update(gameClock, channel, coordinator);
    locationGameSystem.Update(gameClock, channel, coordinator);

    payload = renderCollector.PayloadStager().GetPayload();
    CHECK_EQUAL(0, payload->mLocations.size());
    CHECK_EQUAL(kMaxItems, payload->mActiveIds.size());

    for (auto id : ids)
    {
        coordinator.RemoveComponents(id);
    }

    coordinator.RemoveComponents(physWorldId);
}
