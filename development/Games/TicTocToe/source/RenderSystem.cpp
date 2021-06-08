#include "RenderSystem.h"
#include "cpp-terminal/terminal.h"

ttt::RenderSystem::RenderSystem(Messaging& messaging, yaget::Application& app)
    : GameSystem("RenderSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); })
    , mTerminal(std::make_unique<Term::Terminal>())
{
    mTerminal->save_screen();

    int rows, cols;
    mTerminal->get_term_size(rows, cols);

    int width = cols/2;
    int height = rows;

    Term::Window window(10, 1, width, height);
    window.print_border();
    const std::string& welcomeBanner1 = "Welcome to Tic-Toc-Toe";
    const std::string& welcomeBanner2 = "May the Best Mind Win";
    const int posX1 = static_cast<int>(width / 2 - welcomeBanner1.length()/2);
    const int posX2 = static_cast<int>(width / 2 - welcomeBanner2.length()/2);

    const std::string underline(width, '-');

    window.print_str(posX1, 2, welcomeBanner1);
    window.print_str(posX2, 3, welcomeBanner2);
    window.print_str(2, 4, underline);

    const auto& winRender = window.render();
    mTerminal->write(winRender);
}

ttt::RenderSystem::~RenderSystem()
{
    mTerminal->restore_screen();
}

void ttt::RenderSystem::OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, RenderComponent* renderComponent)
{
    int z = 0;
    z;
    id;
    gameClock;
    channel;
    renderComponent;
}
