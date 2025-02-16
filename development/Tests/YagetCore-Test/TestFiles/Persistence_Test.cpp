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
        CComponent(comp::Id_t id, const db_c::Param::Types& param)
            : PersistentBaseComponent(id, std::tie(param))
        {
        }
    };
    
    using Entity = comp::RowPolicy<AComponent*, BComponent*, CComponent*>;
    using EntityCoordinator = comp::Coordinator<Entity>;
    using GameCoordinatorSet = comp::CoordinatorSet<EntityCoordinator>;
    using StagerSystem = items::StagerSystem<GameCoordinatorSet, Messaging>;

    using SystemsCoordinator = comp::gs::SystemsCoordinator<GameCoordinatorSet, Messaging, Application, StagerSystem>;

    using SetupDirector = items::SetupDirector<SystemsCoordinator>;
    using Director = items::DefaultDirector<SystemsCoordinator>;
}


class Persistence : public testing::Test
{
    //CHECK_EQUAL(expected, actual);
    //yaget::test::Environment mEnvironment;
};


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

        component->GetValue<db_a::Param>() = aParam2;
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
