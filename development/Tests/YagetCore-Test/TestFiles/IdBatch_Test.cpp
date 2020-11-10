#include "pch.h"
#include "Items/ItemsDirector.h"
#include "TestHelpers/TestHelpers.h"

class IdBatch : public ::testing::Test
{
private:
	yaget::test::Environment mEnvironment;
};


TEST_F(IdBatch, Burnable)
{
	using namespace yaget;
	
	IdGameCache idGameCache(nullptr);

	auto id = comp::ItemId{ comp::PERSISTENT_ID_BIT };
	EXPECT_FALSE(comp::IsIdValid(id));
	EXPECT_FALSE(comp::IsIdPersistent(id));
	id = comp::StripQualifiers(id);
	EXPECT_FALSE(comp::IsIdValid(id));
	EXPECT_FALSE(comp::IsIdPersistent(id));

	id = comp::ItemId{ comp::INVALID_ID };
	EXPECT_FALSE(comp::IsIdValid(id));
	EXPECT_FALSE(comp::IsIdPersistent(id));
	id = comp::StripQualifiers(id);
	EXPECT_FALSE(comp::IsIdValid(id));
	EXPECT_FALSE(comp::IsIdPersistent(id));

	id = comp::ItemId{ comp::END_ID_MARKER };
	EXPECT_FALSE(comp::IsIdValid(id));
	EXPECT_FALSE(comp::IsIdPersistent(id));
	id = comp::StripQualifiers(id);
	EXPECT_FALSE(comp::IsIdValid(id));
	EXPECT_FALSE(comp::IsIdPersistent(id));

	id = comp::ItemId{ comp::GLOBAL_ID_MARKER };
	EXPECT_FALSE(comp::IsIdValid(id));
	EXPECT_FALSE(comp::IsIdPersistent(id));
	id = comp::StripQualifiers(id);
	EXPECT_FALSE(comp::IsIdValid(id));
	EXPECT_FALSE(comp::IsIdPersistent(id));

	id = comp::ItemId{ 1 };
	EXPECT_TRUE(comp::IsIdValid(id));
	EXPECT_FALSE(comp::IsIdPersistent(id));

	id = comp::MarkAsPersistent(id);
	EXPECT_TRUE(comp::IsIdValid(id));
	EXPECT_TRUE(comp::IsIdPersistent(id));
	
	id = comp::StripQualifiers(id);
	EXPECT_TRUE(comp::IsIdValid(id));
	EXPECT_FALSE(comp::IsIdPersistent(id));

	const auto MaxIterations = 10000;
	std::set<comp::ItemId> itemIds;

	for (int i = 0; i < MaxIterations; ++i)
	{
		comp::ItemId itemId = idGameCache.GetId(IdGameCache::IdType::Burnable);
		EXPECT_TRUE(itemIds.insert(itemId).second);
	}

	std::set<comp::ItemId>().swap(itemIds);
	
	for (int i = 0; i < MaxIterations; ++i)
	{
		comp::ItemId itemId = idGameCache.GetId(IdGameCache::IdType::Persistent);
		EXPECT_TRUE(itemIds.insert(itemId).second);
	}
}

TEST_F(IdBatch, Persistent)
{
	using namespace yaget;

	items::NamedDirector<comp::db::EmptySchema> director("Persistent");
	auto& idGameCache = director.IdCache();

	const auto MaxIterations = 10000;
	std::set<comp::ItemId> itemIds;

	{
		const auto& message = fmt::format("Getting '{}' Burnable id's.", conv::ToThousandsSep(MaxIterations));
		metrics::TimeScoper<time::kMilisecondUnit> intTimer("TEST", message.c_str(), YAGET_LOG_FILE_LINE_FUNCTION);

		for (int i = 0; i < MaxIterations; ++i)
		{
			comp::ItemId itemId = idspace::get_burnable(idGameCache);
			EXPECT_TRUE(itemIds.insert(itemId).second);
		}
	}

	std::set<comp::ItemId>().swap(itemIds);

	{
		const auto& message = fmt::format("Getting '{}' Persistent id's.", conv::ToThousandsSep(MaxIterations));
		metrics::TimeScoper<time::kMilisecondUnit> intTimer("TEST", message.c_str(), YAGET_LOG_FILE_LINE_FUNCTION);
		
		for (int i = 0; i < MaxIterations; ++i)
		{
			comp::ItemId itemId = idspace::get_persistent(idGameCache);
			EXPECT_TRUE(itemIds.insert(itemId).second);
		}
	}
}
