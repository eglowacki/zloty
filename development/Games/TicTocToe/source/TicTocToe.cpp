// TicTocToe.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Components/GameCoordinatorGenerator.h>

#include "YagetCore.h"
#include "YagetVersion.h"

#include "App/AppHarness.h"
#include "App/ConsoleApplication.h"

#include "Items/ItemsDirector.h"

#include "LoggerCpp/OutputDebug.h"
#include "LoggerCpp/OutputFile.h"

#include "VTS/DiagnosticVirtualTransportSystem.h"
#include "VTS/ToolVirtualTransportSystem.h"


namespace yaget::ylog
{
  yaget::Strings GetRegisteredTags()
  {
      yaget::Strings tags =
      {
          #include "Logger/LogTags.h"
      };

      return tags;
  }
} // namespace yaget::ylog

YAGET_BRAND_NAME("Beyond Limits")

namespace 
{
    const auto configBlock = R"(
    {
        "Configuration" : {
            "Debug": {
                "Logging": {
                    "Filters": [ "MULT", "POOL", "SQL" ],
                    "Level": "DBUG",
                    "Outputs": {
                        "ylog::OutputDebug": {
                            "split_lines": "true"
                        },
                        "ylog::OutputFile": {
                            "max_startup_size": "0",
                            "filename": "$(LogFolder)/$(AppName).log"
                        }
                    }
                }
            },

            "Init": {
                "Aliases": {
                   "$(AssetsFolder) ": {
                        "Path": "$(UserDataFolder)/Assets",
                        "ReadOnly": true
                    },
                   "$(DatabaseFolder) ": {
                        "Path": "$(UserDataFolder)/Database",
                        "ReadOnly": true
                    }
                }
            }
        }
    }
    )";
}



int main(int argc, char* argv[])
{
    YAGET_CHECKVERSION;

    using namespace yaget;
    using Section = io::VirtualTransportSystem::Section;

    args::Options options("Yaget.TicTocToe", "Usage of core yaget library to build a game. Eat your own dog food.");

    int result = app::helpers::Harness<ylog::OutputFile, ylog::OutputDebug>(argc, argv, options, configBlock, std::strlen(configBlock), [&options]()
    {
        if (options.find<bool>("vts_fix", false))
        {
            yaget::io::diag::VirtualTransportSystem vtsFixer(false, "$(DatabaseFolder)/vts.sqlite");
        }

        io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, {}, "$(DatabaseFolder)/vts.sqlite");

        int64_t schemaVersion = Database::NonVersioned;
        //const auto& pongerSchema = comp::db::GenerateGameDirectorSchema<pong::GameCoordinator>(schemaVersion);
        items::Director director("$(DatabaseFolder)/director.sqlite", {}, schemaVersion);

        app::DefaultConsole app("Yaget.Tic-Tac-Toe", director, vts, options);

        return 0;
    });

    return result;

    ///*auto result =*/ system::InitializeSetup();
    //DefaultConsole(const std::string & title, items::Director & director, io::VirtualTransportSystem & vts, const args::Options & options);
}
