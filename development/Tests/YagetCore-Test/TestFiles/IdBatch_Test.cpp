#include "pch.h"
#include "Items/ItemsDirector.h"
#include "TestHelpers/TestHelpers.h"

class IdBatch : public ::testing::Test
{
};


TEST_F(IdBatch, Burnable)
{
	using namespace yaget;
	
	IdGameCache idGameCache(nullptr);

	auto id = comp::Id_t{ comp::PERSISTENT_ID_BIT };
	EXPECT_FALSE(comp::IsIdValid(id));
	EXPECT_FALSE(comp::IsIdPersistent(id));
	id = comp::StripQualifiers(id);
	EXPECT_FALSE(comp::IsIdValid(id));
	EXPECT_FALSE(comp::IsIdPersistent(id));

	id = comp::Id_t{ comp::INVALID_ID };
	EXPECT_FALSE(comp::IsIdValid(id));
	EXPECT_FALSE(comp::IsIdPersistent(id));
	id = comp::StripQualifiers(id);
	EXPECT_FALSE(comp::IsIdValid(id));
	EXPECT_FALSE(comp::IsIdPersistent(id));

	id = comp::Id_t{ comp::END_ID_MARKER };
	EXPECT_FALSE(comp::IsIdValid(id));
	EXPECT_FALSE(comp::IsIdPersistent(id));
	id = comp::StripQualifiers(id);
	EXPECT_FALSE(comp::IsIdValid(id));
	EXPECT_FALSE(comp::IsIdPersistent(id));

	id = comp::Id_t{ comp::GLOBAL_ID_MARKER };
	EXPECT_FALSE(comp::IsIdValid(id));
	EXPECT_FALSE(comp::IsIdPersistent(id));
	id = comp::StripQualifiers(id);
	EXPECT_FALSE(comp::IsIdValid(id));
	EXPECT_FALSE(comp::IsIdPersistent(id));

	id = comp::Id_t{ 1 };
	EXPECT_TRUE(comp::IsIdValid(id));
	EXPECT_FALSE(comp::IsIdPersistent(id));

	id = comp::MarkAsPersistent(id);
	EXPECT_TRUE(comp::IsIdValid(id));
	EXPECT_TRUE(comp::IsIdPersistent(id));
	
	id = comp::StripQualifiers(id);
	EXPECT_TRUE(comp::IsIdValid(id));
	EXPECT_FALSE(comp::IsIdPersistent(id));
}

TEST_F(IdBatch, Persistent)
{
	using namespace yaget;

	//items::DefaultDirector

}
