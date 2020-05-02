#include "Timer/ClockManager.h"
#include "Timer/ITimeSource.h"
#include "Win32/TimeSourceHRWin32.h"
#include <cstddef>
#include <vector>
#include <deque>
#include <algorithm>

#define NOMINMAX
#include <windows.h>

namespace
{
    typedef std::vector<eg::IObserver *> ObserverList;

    /*!
    This is used here as a pimple patter, so I do not need to export stl containers,
    since VC 7.1 still does not work correctly.
    */
    class PrivateClockData
    {
    public:
        std::deque<double> mFrameDurationHistory;
        ObserverList mObservers;
    };

} // namespace

namespace eg {



ClockManager::ClockManager(const ITimeSource *pSource) :
    mpTimeSource(0),
    mCurrentTime(0),
    mFrameTime(0),
    mFrameNumber(0),
    mSourceStartValue(0),
    mSourceLastValue(0),
    mClockData(new PrivateClockData)
{
    if (pSource)
    {
        SetTimeSource(pSource);
    }
    else
    {
        SetTimeSource(new TimeSourceHRWin32);
    }

    SetFiltering(10);
    mFrameTime = GetPredictedFrameDuration();
}


ClockManager::~ClockManager()
{
    delete mpTimeSource;
    delete mClockData;
}


double ClockManager::GetRealTime() const
{
    if (mpTimeSource)
    {
        double currentTime = mpTimeSource->GetTime();
        return currentTime - mSourceStartValue;
    }

    return 0;
}

void ClockManager::SetTimeSource(const ITimeSource *pSource)
{
    delete mpTimeSource;
    mpTimeSource = pSource;
    if (mpTimeSource)
    {
        mSourceStartValue = mpTimeSource->GetTime();
        mSourceLastValue = mSourceStartValue;
    }
}



void ClockManager::FrameStep()
{
    double exactLastFrameDuration = GetExactLastFrameDuration();
    AddToFrameHistory(exactLastFrameDuration);

    mFrameTime = GetPredictedFrameDuration();
    mCurrentTime += mFrameTime;

    mFrameNumber++;

    ObserverList::iterator it;
    for (it = mClockData->mObservers.begin(); it != mClockData->mObservers.end(); ++it)
    {
        (*it)->Notify();
    }
}


double ClockManager::GetExactLastFrameDuration()
{
    double sourceTime;
    if (!mpTimeSource)
    {
        sourceTime = 0;
    }
    else
    {
        sourceTime = mpTimeSource->GetTime();
    }

    double frameDuration = sourceTime - mSourceLastValue;
    if (frameDuration > 0.200)
    {
        frameDuration = mClockData->mFrameDurationHistory.back();
    }

    mSourceLastValue = sourceTime;
    return frameDuration;
}


void ClockManager::AddToFrameHistory(double exactFrameDuration)
{
    mClockData->mFrameDurationHistory.push_back(exactFrameDuration);
    if (mClockData->mFrameDurationHistory.size () > (unsigned int) mFrameFilteringWindow)
    {
        mClockData->mFrameDurationHistory.pop_front();
    }
}


double ClockManager::GetPredictedFrameDuration() const
{
    double totalFrameTime = 0;

    std::deque<double>::const_iterator it;
    for (it = mClockData->mFrameDurationHistory.begin(); it != mClockData->mFrameDurationHistory.end(); ++it)
    {
        totalFrameTime += *it;
    }

    return totalFrameTime/mClockData->mFrameDurationHistory.size();
}



void ClockManager::AddObserver(IObserver *pObserver)
{
    if (pObserver)
    {
        mClockData->mObservers.push_back(pObserver);
    }
}


void ClockManager::RemoveObserver(IObserver *pObserver)
{
    mClockData->mObservers.erase(std::remove(mClockData->mObservers.begin(), mClockData->mObservers.end(), pObserver),
                                 mClockData->mObservers.end());

}


float ClockManager::GetFrameRate() const
{
    return 1.0f/(float)mFrameTime;
}


void ClockManager::SetFiltering(int frameWindow, double frameDefault)
{
    mFrameFilteringWindow = frameWindow > 1 ? frameWindow : 1;
    mFrameDefaultTime = frameDefault;
    mClockData->mFrameDurationHistory.clear();
    mClockData->mFrameDurationHistory.push_back(mFrameDefaultTime);
}


double ClockManager::GetTime() const
{
    return mCurrentTime;
}


double ClockManager::GetFrameDuration() const
{
    return mFrameTime;
}


int ClockManager::GetFrameNumber() const
{
    return mFrameNumber;
}

} // namespace eg

