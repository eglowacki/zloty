#include "VTS/DiagnosticVirtualTransportSystem.h"
#include "Logger/YLog.h"
#include "App/FileUtilities.h"

#include <filesystem>


yaget::io::diag::VirtualTransportSystem::VirtualTransportSystem(bool diagnosticMode, const char* fileName)
    : io::VirtualTransportSystem(diagnosticMode ? RuntimeMode::Diagnostic : RuntimeMode::Fix, fileName)
{
    YLOG_INFO("VTSD", "Virtual Transport System in %s mode.", mRuntimeMode == RuntimeMode::Diagnostic ? "diagnostic" : "fix");

    YLOG_INFO("VTSD", "====================== 'DirtyTags' table verification ======================");
    const bool validDirtyTags = VerifyDirtyTags();
    YLOG_INFO("VTSD", "'DirtyTags' table %s validation.", validDirtyTags ? "passed" : "failed");

    if (mRuntimeMode == RuntimeMode::Fix)
    {
        if (DatabaseHandle dHandle = LockDatabaseAccess())
        {
            if (!validDirtyTags)
            {
                YLOG_INFO("VTSD", "Fixing table 'DirtyTags'...");
                bool result = dHandle->DB().ExecuteStatement("DELETE FROM 'DirtyTags';", nullptr);
                YAGET_ASSERT(result, "Did not delete 'DirtyTags' table.");
            }
        }
    }
    else
    {
        if (!validDirtyTags)
        {
            YLOG_ERROR("VTSD", "VTS has errors, run in Fix mode to cleanup DB.");
        }
        else
        {
            YLOG_INFO("VTSD", "VTS DB passed validation OK.");
        }
    }
}


bool yaget::io::diag::VirtualTransportSystem::VerifyDirtyTags() const
{
    int numDirtyBlobs = 0;
    if (DatabaseHandle dHandle = LockDatabaseAccess())
    {
        if (numDirtyBlobs = GetCell<int>(dHandle->DB(), "SELECT COUNT(*) FROM 'DirtyTags';"); numDirtyBlobs)
        {
            using DirtyRow = std::tuple<Guid /*guid*/, std::string /*name*/, std::string /*VTS*/, std::string /*Section*/>;
            std::vector<DirtyRow> dirtyBlobs = dHandle->DB().GetRowsTuple<DirtyRow>("SELECT Tags.Guid, Tags.Name, Tags.VTS, Tags.Section FROM Tags INNER JOIN DirtyTags ON Tags.Guid=DirtyTags.Guid;");

            for (const auto& it : dirtyBlobs)
            {
                std::string fileName = util::ExpendEnv(std::get<2>(it), nullptr);
                const bool fileExist = io::file::IsFileExists(fileName);
                const char* fileMessage = fileExist ? "exists" : "does not exist";
                YLOG_INFO("VTSD", "    File '%s' under Section: '%s' marked as dirty, %s on disk. Path: '%s'.", std::get<1>(it).c_str(), std::get<3>(it).c_str(), fileMessage, fileName.c_str());
            }
        }
    }

    return numDirtyBlobs == 0;
}



