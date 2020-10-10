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
#include "VTS/ResolvedAssets.h"
#include "VTS/ToolVirtualTransportSystem.h"


namespace yaget::ylog
{
  yaget::Strings GetRegisteredTags()
  {
      yaget::Strings tags =
      {
          #include "Logger/LogTags.h"
          "SPAM"
      };

      return tags;
  }
} // namespace yaget::ylog

YAGET_BRAND_NAME("Beyond Limits")

namespace 
{
    const auto configBlock = R"###(
    {
        "Configuration" : {
            "Debug": {
                "Logging": {
                    "Filters": [ "MULT", "POOL", "SQL", "FILE", "VTS", "IDS", "INPT", "INIT", "SPAM" ],
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
                   "$(AssetsFolder)": {
                        "Path": "$(UserDataFolder)/Assets",
                        "ReadOnly": true
                    },
                   "$(DatabaseFolder)": {
                        "Path": "$(UserDataFolder)/Database",
                        "ReadOnly": true
                    }
                },
                "VTS" : [{
                    "Settings": {
                        "Converters": "JSON",
                        "Filters" : [ "*.json" ],
                        "Path" : [ "$(DataFolder)/Data", "$(AppFolder)" ],
                        "ReadOnly" : true,
                        "Recursive" : false
                    }
                }]
            }
        }
    }
    )###";


    //using UpdateCallback_t = std::function<void(Application&, const time::GameClock&, metrics::Channel&)>;

    class GameDirector
    {
    public:
        GameDirector()
            : mStates({ 
                {GameState::Welcome, [this](auto&&... params) { WelcomeLoop(params...); }},
                {GameState::Menu, [this](auto&&... params) { MenuLoop(params...); }},
                {GameState::NewGame, [this](auto&&... params) { NewGameLoop(params...); }},
                {GameState::FirstPlayer, [this](auto&&... params) { FirstPlayerLoop(params...); }},
                {GameState::GameTick, [this](auto&&... params) { GameTickLoop(params...); }}
            })
        {
        }

        void GameLoop(yaget::Application& app , const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel)
        {
            mStates[mCurrentState](app, gameClock, channel);
        }

    private:
        void WelcomeLoop(yaget::Application& app, const yaget::time::GameClock&, yaget::metrics::Channel&)
        {
            app.Input().PushContext("BLANK");
            std::cout << "Welcome to Tic-Toc-Toe Game!\n";
            mCurrentState = GameState::Menu;
        }

        void MenuLoop(yaget::Application& app, const yaget::time::GameClock&, yaget::metrics::Channel&)
        {
            if (!mMenuState.mMenuRendered)
            {
                mMenuState.mMenuRendered = true;
                app.Input().PushContext("MENU");

                std::cout << "Main Menu\n";
                std::cout << app.Input().ActionToString("New Game") <<  ". New Game\n";
                std::cout << app.Input().ActionToString("Resume Game") << ". Resume Game\n";
                std::cout << app.Input().ActionToString("Save Game") << ". Save Game\n";
                std::cout << app.Input().ActionToString("Load Game") << ". Load Game\n";
                std::cout << app.Input().ActionToString("Options") << ". Options\n";
                std::cout << app.Input().ActionToString("Quit App") << ". Quit Game\n";

                app.Input().RegisterSimpleActionCallback("New Game", [this, &app]()
                {
                    app.Input().PopContext();

                    mMenuState.mMenuRendered = false;
                    mCurrentState = GameState::NewGame;
                });
                app.Input().RegisterSimpleActionCallback("Resume Game", []()
                {
                    std::cout << "Resume Game Selected\n";
                });
                app.Input().RegisterSimpleActionCallback("Save Game", []()
                {
                    std::cout << "Save Game Selected\n";
                });
                app.Input().RegisterSimpleActionCallback("Load Game", []()
                {
                    std::cout << "Load Game Selected\n";
                });
                app.Input().RegisterSimpleActionCallback("Options", []()
                {
                    std::cout << "Option Selected\n";
                });
            }
        }

        void NewGameLoop(yaget::Application& app, const yaget::time::GameClock&, yaget::metrics::Channel&)
        {
            if (!mMenuState.mMenuRendered)
            {
                mMenuState.mMenuRendered = true;
                mMenuState.mNumPlayers = 0;
                app.Input().PushContext("MENU.NEW_GAME");

                std::cout << "New Game:\n";
                std::cout << "    How many players:\n";
                std::cout << "        " << app.Input().ActionToString("One Player") << ". One Player\n";
                std::cout << "        " << app.Input().ActionToString("Two Players") << ". Two Players\n";

                app.Input().RegisterSimpleActionCallback("One Player", [this]()
                {
                    std::cout << "One Player Selected\n";
                    mMenuState.mNumPlayers = 1;
                });
                app.Input().RegisterSimpleActionCallback("Two Players", [this]()
                {
                    std::cout << "Two Players Selected\n";
                    mMenuState.mNumPlayers = 2;
                });
            }

            if (mMenuState.mNumPlayers)
            {
                app.Input().PopContext();

                std::cout << "Your opponent is " << (mMenuState.mNumPlayers == 1 ? "a super computer\n" : "another human (boring)\n");
                std::cout << "X is your piece and your opponent uses O\n";

                mCurrentState = GameState::FirstPlayer;
                mMenuState.mMenuRendered = false;
            }
        }

        void FirstPlayerLoop(yaget::Application&, const yaget::time::GameClock&, yaget::metrics::Channel&)
        {
            std::cout << "Choosing who is going to go first...\n";
            const auto index = yaget::platform::GetRandom(1, 2);

            mCurrentPlayer = index == 1 ? 'X' : 'O';
            std::cout << "Player who controls " << mCurrentPlayer << " starts\n";

            mCurrentState = GameState::GameTick;
        }

        void GameTickLoop(yaget::Application& app, const yaget::time::GameClock&, yaget::metrics::Channel&)
        {
            if (!mMenuState.mMenuRendered)
            {
                mTurn = 1;
                for (int i = 0; i < 9; ++i)
                {
                    mPositions[i] = static_cast<char>(i)+'1';
                }

                auto squareInput = [this, &app](const std::string& actionName, uint64_t /*timeStamp*/, int32_t /*mouseX*/, int32_t /*mouseY*/, uint32_t /*flags*/)
                {
                    int selectedSquare = -1;
                    for (auto i = 0; i < 9; ++i)
                    {
                        if (actionName == "Board.Square " + std::to_string(i+1))
                        {
                            selectedSquare = i;
                            break;
                        }
                    }

                    if (IsMoveValid(selectedSquare))
                    {
                        mTurn++;
                        mPositions[selectedSquare] = mCurrentPlayer;

                        std::cout << selectedSquare + 1 << "\n";
                        PrintBoard();

                        if (mTurn == 10)
                        {
                            std::cout << "*******************************************************************************\n";
                            std::cout << "Draw, neither player won\n";
                            std::cout << "*******************************************************************************\n";
                            app.Input().PopContext();

                            mMenuState.mMenuRendered = false;
                            mCurrentState = GameState::Menu;
                        }
                        else if (IsOne())
                        {
                            std::cout << "*******************************************************************************\n";
                            std::cout << "Congratulations " << mCurrentPlayer << " Player. You won in " << mTurn << " turns and beat Player " << (mCurrentPlayer == 'X' ? 'O' : 'X') << "\n";
                            std::cout << "*******************************************************************************\n";
                            app.Input().PopContext();

                            mMenuState.mMenuRendered = false;
                            mCurrentState = GameState::Menu;
                        }
                        else
                        {
                            mCurrentPlayer = mCurrentPlayer == 'X' ? 'O' : 'X';
                            std::cout << "Select which square number to place your " << mCurrentPlayer << " piece on: ";
                        }
                    }
                    
                };

                for (auto i = 0; i < 9; ++i)
                {
                    app.Input().RegisterActionCallback("Board.Square " + std::to_string(i + 1), squareInput);
                }

                mMenuState.mMenuRendered = true;
                app.Input().PushContext("GAME");
                PrintBoard();
                std::cout << "Select which square number to place your " << mCurrentPlayer << " piece on: ";
            }

        }

        void PrintBoard() const
        {
            std::cout << "Turn: " << mTurn << "\n";
            std::cout << "       |       |       \n";
            std::cout << "   " << mPositions[0] << "   |   " << mPositions[1] << "   |   " << mPositions[2] << "   \n";
            std::cout << "       |       |       \n";
            std::cout << "-------+-------+-------\n";
            std::cout << "       |       |       \n";
            std::cout << "   " << mPositions[3] << "   |   " << mPositions[4] << "   |   " << mPositions[5] << "   \n";
            std::cout << "       |       |       \n";
            std::cout << "-------+-------+-------\n";
            std::cout << "       |       |       \n";
            std::cout << "   " << mPositions[6] << "   |   " << mPositions[7] << "   |   " << mPositions[8] << "   \n";
            std::cout << "       |       |       \n";
        }

        // is move valid or that sqare is already taken
        bool IsMoveValid(int move) const
        {
            return move > -1 && move < 9 && mPositions[move] != 'X' && mPositions[move] != 'O';

        }

        // is this board has winning condtition
        bool IsOne() const
        {
            int WiningConditions[][3] = {
                {0, 1, 2},
                {3, 4, 5},
                {6, 7, 8},

                {0, 3, 6},
                {1, 4, 7},
                {2, 5, 8},

                {0, 4, 8},
                {2, 4, 6}
            };

            for (auto v : WiningConditions)
            {
                if (mPositions[v[0]] == mPositions[v[1]] && mPositions[v[0]] == mPositions[v[2]])
                {
                    return true;
                }
            }


            return false;
        }

        enum class GameState { Welcome, Menu, NewGame, FirstPlayer, GameTick };
        GameState mCurrentState = GameState::Welcome;

        using States = std::map<GameState, yaget::Application::UpdateCallback_t>;
        States mStates;

        struct MenuState
        {
            bool mMenuRendered = false;
            std::atomic_int mNumPlayers = { 0 };
        };
        MenuState mMenuState;

        char mPositions[9] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
        char mCurrentPlayer = '\0';
        int mTurn = 0;
    };
}



int main(int argc, char* argv[])
{
    YAGET_CHECKVERSION;

    using namespace yaget;
    using Section = io::VirtualTransportSystem::Section;

    args::Options options("Yaget.TicTocToe", "Usage of core yaget library to build a game. Eat your own dog food.");

    int result = app::helpers::Harness<ylog::OutputFile, ylog::OutputDebug>(argc, argv, options, nullptr/*configBlock*/, 0/*std::strlen(configBlock)*/, [&options]()
    {
        if (options.find<bool>("vts_fix", false))
        {
            yaget::io::diag::VirtualTransportSystem vtsFixer(false, "$(DatabaseFolder)/vts.sqlite");
        }

        const io::VirtualTransportSystem::AssetResolvers resolvers = {
            { "JSON", io::ResolveAsset<io::JsonAsset> }
        };

        io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, resolvers, "$(DatabaseFolder)/vts.sqlite");

        const int64_t schemaVersion = Database::NonVersioned;
        //const auto& pongerSchema = comp::db::GenerateGameDirectorSchema<pong::GameCoordinator>(schemaVersion);
        items::Director director("$(DatabaseFolder)/director.sqlite", {}, schemaVersion);

        app::DefaultConsole app("Yaget.Tic-Tac-Toe", director, vts, options);

        GameDirector gameDirector;
        auto logicCallback = [&gameDirector](auto&&... params) { gameDirector.GameLoop(params...); };

        const auto result = app.Run(logicCallback, {}, {}, {}, {});
        return result;
    });

    return result;

    ///*auto result =*/ system::InitializeSetup();
    //DefaultConsole(const std::string & title, items::Director & director, io::VirtualTransportSystem & vts, const args::Options & options);
}
