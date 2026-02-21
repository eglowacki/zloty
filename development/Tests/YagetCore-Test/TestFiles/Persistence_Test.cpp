#include "pch.h" 
#include "Components/Coordinator.h"
#include "Components/CoordinatorSet.h"
#include "Components/PersistentBaseComponent.h"
#include "Components/SystemsCoordinator.h"

#include "GameSystem/Messaging.h"

#include "Items/StagerSystem.h"

#include "TestHelpers/TestHelpers.h"


namespace 
{
    using namespace yaget;

    using Messaging = comp::gs::Messaging<std::shared_ptr<char>>;

    namespace db_a
    {
        struct Param { using Types = int; };
        using ValueTypes = std::tuple<Param>;
    }

    class AComponent : public comp::db::PersistentBaseComponent<db_a::ValueTypes>
    {
    public:
        AComponent(comp::Id_t id, const db_a::Param::Types& param)
            : PersistentBaseComponent(id, std::tie(param))
        {
        }
    };
    
    namespace db_b
    {
        struct Param { using Types = std::string; };
        using ValueTypes = std::tuple<Param>;
    }

    class BComponent : public comp::db::PersistentBaseComponent<db_b::ValueTypes>
    {
    public:
        BComponent(comp::Id_t id, const db_b::Param::Types& param)
            : PersistentBaseComponent(id, std::tie(param))
        {
        }
    };
    
    namespace db_c
    {
        struct Param { using Types = float; };
        using ValueTypes = std::tuple<Param>;
    }

    class CComponent : public comp::db::PersistentBaseComponent<db_c::ValueTypes>
    {
    public:
        CComponent(comp::Id_t id, const db_c::Param::Types& param = {})
            : PersistentBaseComponent(id, std::tie(param))
        {
        }
    };

    namespace db_d
    {
        struct ParamInt { using Types = int; };
        struct ParamString { using Types = std::string; };
        using ValueTypes = std::tuple<ParamInt, ParamString>;
    }

    class DComponent : public comp::db::PersistentObserverComponent<db_d::ValueTypes>
    {
    public:
        DComponent(comp::Id_t id, const db_d::ParamInt::Types& param1, const db_d::ParamString::Types& param2)
            : PersistentObserverComponent(id, std::tie(param1, param2))
        {
        }
    };
    
    using Entity = comp::RowPolicy<AComponent*, BComponent*, CComponent*, DComponent*, items::StageComponent*>;
    using EntityCoordinator = comp::Coordinator<Entity>;
    using GameCoordinatorSet = comp::CoordinatorSet<EntityCoordinator>;
    class StagerSystem : public items::StagerSystem<GameCoordinatorSet, Messaging>
    {
    public:
        StagerSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet, bool tickEnabled = true)
            : yaget::items::StagerSystem<GameCoordinatorSet, Messaging>("TestStagerSystem", messaging, app, coordinatorSet, tickEnabled)
        {}
    };

    using SystemsCoordinator = comp::gs::SystemsCoordinator<GameCoordinatorSet, Messaging, Application, StagerSystem>;

    using SetupDirector = items::SetupDirector<SystemsCoordinator>;
    using Director = items::DefaultDirector<SystemsCoordinator>;
}


class Persistence : public testing::Test
{
    //CHECK_EQUAL(expected, actual);
    //yaget::test::Environment mEnvironment;
};


TEST_F(Persistence, Observers)
{
    using namespace yaget;

    const db_d::ParamInt::Types aParam1 = 417;
    const db_d::ParamString::Types aParam2 = "FooBar";
    const db_d::ParamInt::Types aParam1a = 310;
    const db_d::ParamString::Types aParam2a = "Lublin";
    const db_d::ParamInt::Types aParam1b = 1962;
    const db_d::ParamString::Types aParam2b = "Paris";
    
    test::ApplicationFramework<Messaging, SetupDirector, SystemsCoordinator> testerFramework("Persistence.Observers");
    auto& idCache = testerFramework.Ids();
    auto& systemsCoordinator = testerFramework.SystemsCoordinator();

    const auto componentId = idspace::get_persistent(idCache);
    auto component = systemsCoordinator.AddComponent<DComponent>(componentId, aParam1, aParam2);

    EXPECT_TRUE(component != nullptr);
	EXPECT_EQ(componentId, component->Id());
	EXPECT_EQ(aParam1, component->GetValue<db_d::ParamInt>());
	EXPECT_EQ(aParam2, component->GetValue<db_d::ParamString>());

    int counter = 0;
    auto observer = [&counter](auto oldValue, auto newValue)
    {
        counter++;
    };

    // test observer callback when modifying values
    component->Connect<db_d::ParamInt>(observer);
    component->Connect<db_d::ParamString>(observer);

    component->SetValue<db_d::ParamInt>(aParam1a);
	EXPECT_EQ(1, counter);
    component->SetValue<db_d::ParamString>(aParam2a);
	EXPECT_EQ(2, counter);

    // modify storage values
    auto storage = component->GetStorage();
    std::get<db_d::ParamInt::Types>(storage) = aParam1b;
    EXPECT_TRUE(component->SetStorage(storage));
	EXPECT_EQ(3, counter);
	EXPECT_EQ(aParam1b, component->GetValue<db_d::ParamInt>());

    std::get<db_d::ParamString::Types>(storage) = aParam2b;
    EXPECT_TRUE(component->SetStorage(storage));
	EXPECT_EQ(4, counter);
	EXPECT_EQ(aParam2b, component->GetValue<db_d::ParamString>());

    std::get<db_d::ParamInt::Types>(storage) = aParam1a;
    std::get<db_d::ParamString::Types>(storage) = aParam2a;
    EXPECT_TRUE(component->SetStorage(storage));
	EXPECT_EQ(6, counter);
	EXPECT_EQ(aParam1a, component->GetValue<db_d::ParamInt>());
	EXPECT_EQ(aParam2a, component->GetValue<db_d::ParamString>());

    EXPECT_FALSE(component->SetStorage(storage));

    // remove observer(s) and modify values
    component->Connect<db_d::ParamInt>({});
    component->SetValue<db_d::ParamInt>(aParam1a);
	EXPECT_EQ(6, counter);

    component->SetValue<db_d::ParamString>(aParam2b);
	EXPECT_EQ(7, counter);

    component->Connect<db_d::ParamString>({});
    component->SetValue<db_d::ParamInt>(aParam1a);
    component->SetValue<db_d::ParamString>(aParam2b);
	EXPECT_EQ(7, counter);
}


TEST_F(Persistence, SystemsCoordinator)
{
    using namespace yaget;

    comp::Id_t componentId = comp::INVALID_ID;
    constexpr db_a::Param::Types aParam1 = 100;
    constexpr db_a::Param::Types aParam2 = 200;
    const db_b::Param::Types aParam3 = "Foobar";
    const db_c::Param::Types aParam4 = 3.14f;

    {
        test::ApplicationFramework<Messaging, SetupDirector, SystemsCoordinator> testerFramework("Persistence.SystemsCoordinator");
        auto& idCache = testerFramework.Ids();
        auto& systemsCoordinator = testerFramework.SystemsCoordinator();

        componentId = idspace::get_persistent(idCache);
        auto component = systemsCoordinator.AddComponent<AComponent>(componentId, aParam1);

        EXPECT_TRUE(component != nullptr);
	    EXPECT_EQ(componentId, component->Id());
	    EXPECT_EQ(aParam1, component->GetValue<db_a::Param>());
        EXPECT_TRUE(systemsCoordinator.SaveComponent(component));
    }

    {
        test::ApplicationFramework<Messaging, Director, SystemsCoordinator> testerFramework("Persistence.SystemsCoordinator");
        auto& systemsCoordinator = testerFramework.SystemsCoordinator();

        auto component = systemsCoordinator.LoadComponent<AComponent>(componentId);
        EXPECT_TRUE(component != nullptr);
	    EXPECT_EQ(componentId, component->Id());
	    EXPECT_EQ(aParam1, component->GetValue<db_a::Param>());

        component->SetValue<db_a::Param>(aParam2);
	    EXPECT_EQ(aParam2, component->GetValue<db_a::Param>());
        EXPECT_TRUE(systemsCoordinator.SaveComponent(component));
    }

    {
        test::ApplicationFramework<Messaging, Director, SystemsCoordinator> testerFramework("Persistence.SystemsCoordinator");
        auto& systemsCoordinator = testerFramework.SystemsCoordinator();

        auto component = systemsCoordinator.LoadComponent<AComponent>(componentId);
        EXPECT_TRUE(component != nullptr);
	    EXPECT_EQ(componentId, component->Id());
	    EXPECT_EQ(aParam2, component->GetValue<db_a::Param>());
    }

    {
        test::ApplicationFramework<Messaging, Director, SystemsCoordinator> testerFramework("Persistence.SystemsCoordinator");
        auto& systemsCoordinator= testerFramework.SystemsCoordinator();

        auto componentB = systemsCoordinator.AddComponent<BComponent>(componentId, aParam3);
        EXPECT_TRUE(componentB != nullptr);
	    EXPECT_EQ(componentId, componentB->Id());
	    EXPECT_EQ(aParam3, componentB->GetValue<db_b::Param>());
        EXPECT_TRUE(systemsCoordinator.SaveComponent(componentB));

        auto componentC = systemsCoordinator.AddComponent<CComponent>(componentId, aParam4);
        EXPECT_TRUE(componentC != nullptr);
	    EXPECT_EQ(componentId, componentC->Id());
	    EXPECT_FLOAT_EQ(aParam4, componentC->GetValue<db_c::Param>());
        EXPECT_TRUE(systemsCoordinator.SaveComponent(componentC));
    }

    {
        test::ApplicationFramework<Messaging, Director, SystemsCoordinator> testerFramework("Persistence.SystemsCoordinator");
        auto& systemsCoordinator = testerFramework.SystemsCoordinator();

        const auto item = systemsCoordinator.LoadItem<std::tuple<AComponent*, BComponent*, CComponent*>>(componentId);

        const auto aComponent = comp::get_component<AComponent>(item);
        EXPECT_TRUE(aComponent != nullptr);
	    EXPECT_EQ(componentId, aComponent->Id());
	    EXPECT_EQ(aParam2, aComponent->GetValue<db_a::Param>());

        const auto bComponent = comp::get_component<BComponent>(item);
        EXPECT_TRUE(bComponent != nullptr);
	    EXPECT_EQ(componentId, bComponent->Id());
	    EXPECT_EQ(aParam3, bComponent->GetValue<db_b::Param>());

        const auto cComponent = comp::get_component<CComponent>(item);
        EXPECT_TRUE(cComponent != nullptr);
	    EXPECT_EQ(componentId, cComponent->Id());
	    EXPECT_FLOAT_EQ(aParam4, cComponent->GetValue<db_c::Param>());
    }

    {
        test::ApplicationFramework<Messaging, Director, SystemsCoordinator> testerFramework("Persistence.SystemsCoordinator");
        auto& systemsCoordinator = testerFramework.SystemsCoordinator();

        const auto item = systemsCoordinator.LoadItem(componentId);

        const auto aComponent = comp::get_component<AComponent>(item);
        EXPECT_TRUE(aComponent != nullptr);
	    EXPECT_EQ(componentId, aComponent->Id());
	    EXPECT_EQ(aParam2, aComponent->GetValue<db_a::Param>());

        const auto bComponent = comp::get_component<BComponent>(item);
        EXPECT_TRUE(bComponent != nullptr);
	    EXPECT_EQ(componentId, bComponent->Id());
	    EXPECT_EQ(aParam3, bComponent->GetValue<db_b::Param>());

        const auto cComponent = comp::get_component<CComponent>(item);
        EXPECT_TRUE(cComponent != nullptr);
	    EXPECT_EQ(componentId, cComponent->Id());
	    EXPECT_FLOAT_EQ(aParam4, cComponent->GetValue<db_c::Param>());
    }
}
