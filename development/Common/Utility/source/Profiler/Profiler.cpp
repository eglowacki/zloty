#include "Profiler/Profiler.h"
#include "Logger/Log.h"


namespace eg {


Profiler::Profiler(InputManager& input)
: mInput(input)
, mbProfilerOn(false)
, mProfilerMode(0)
, mbProfilerGraph(false)
, mCurrentTotal(0)
{
    mProfilerToggle = mInput.ListenForAction("Profiler.Toggle", boost::bind(&Profiler::onAction, this, _1, _2, _3, _4));
    mProfilerReportMode = mInput.ListenForAction("Profiler.ReportMode", boost::bind(&Profiler::onAction, this, _1, _2, _3, _4));
    mProfilerMoveUp = mInput.ListenForAction("Profiler.Cursor.Move.Up", boost::bind(&Profiler::onAction, this, _1, _2, _3, _4));
    mProfilerMoveDown = mInput.ListenForAction("Profiler.Cursor.Move.Down", boost::bind(&Profiler::onAction, this, _1, _2, _3, _4));
    mProfilerSelectZone = mInput.ListenForAction("Profiler.Cursor.Select.Zone", boost::bind(&Profiler::onAction, this, _1, _2, _3, _4));
    mProfilerSelectParent = mInput.ListenForAction("Profiler.Cursor.Select.Parent", boost::bind(&Profiler::onAction, this, _1, _2, _3, _4));
    mProfilerToggleDisplay = mInput.ListenForAction("Profiler.Toggle.Display", boost::bind(&Profiler::onAction, this, _1, _2, _3, _4));
}


Profiler::~Profiler()
{
    Prof_destroy();
}


void Profiler::Tick(float deltaTime)
{
    Prof_update(1);
    mCurrentTotal += deltaTime;
    if (mbProfilerOn && mCurrentTotal > 0.1f)
    {
        mCurrentTotal =- 0.1f;
        if (mbProfilerGraph)
        {
        }
        else
        {
            Prof_draw_console(2*8, 2*8, 76*8, 16*8, 15, 4);
        }
    }
}


void Profiler::onAction(const std::string& actionName, uint32_t /*timeStamp*/, int32_t /*mouseX*/, int32_t /*mouseY*/)
{
    if (actionName == "Profiler.Toggle")
    {
        mbProfilerOn = !mbProfilerOn;
        if (mbProfilerOn)
        {
            mInput.PushContext("Profiler");
        }
        else
        {
            std::string lastContext = mInput.PopContext();
            assert(lastContext == std::string("Profiler"));
        }
    }
    else if (actionName == "Profiler.ReportMode")
    {
        mProfilerMode = ++mProfilerMode % 3;
        log_trace(tr_util) << "CurrentMode: " << mProfilerMode;
        Prof_set_report_mode((Prof_Report_Mode)mProfilerMode);
    }
    else if (actionName == "Profiler.Cursor.Move.Up")
    {
        Prof_move_cursor(-1);
    }
    else if (actionName == "Profiler.Cursor.Move.Down")
    {
        Prof_move_cursor(1);
    }
    else if (actionName == "Profiler.Cursor.Select.Zone")
    {
        Prof_select();
    }
    else if (actionName == "Profiler.Cursor.Select.Parent")
    {
        Prof_select_parent();
    }
    else if (actionName == "Profiler.Toggle.Display")
    {
        mbProfilerGraph = !mbProfilerGraph;
    }
}

} // namespace eg

