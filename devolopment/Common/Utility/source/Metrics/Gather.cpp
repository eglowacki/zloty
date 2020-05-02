#include "Metrics/Gather.h"


yaget::metrics::Gather yaget::metrics::internal::gatherer;

//------------------------------------------------------------------------------------------------------------------------------------------------------
yaget::metrics::Gather::Gather()
{
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
yaget::metrics::Gather::~Gather()
{
}


//--------------------------------------------------------------------------------------------------
void yaget::metrics::Gather::Add(IdMarker idMarker, yaget::time::Raw_t timeValue, const char* name)
{
    YAGET_ASSERT(name, "Add MarkerId: '%d' must have name associated with it.");

    const MarkerData* currentContext = mContextStack.empty() ? nullptr : mContextStack.top();
    currentContext;

    auto it = mMarkers.find(idMarker);
    if (it == mMarkers.end())
    {
        MarkerData markerData{ name, idMarker, (currentContext ? currentContext->mMarkerId : -1) };

        it = mMarkers.insert(std::make_pair(idMarker, markerData)).first;
    }

    if (it->second.mName.empty())
    {
        it->second.mName = name;
    }

    it->second.mTime += timeValue;
    it->second.mHits++;
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
void yaget::metrics::Gather::PushParent(IdMarker idMarker)
{
    const MarkerData* currentContext = mContextStack.empty() ? nullptr : mContextStack.top();
    YAGET_ASSERT((currentContext ? currentContext->mMarkerId != idMarker : true), "MarkerId: '%d' is already set as Parent.", idMarker);

    auto it = mMarkers.find(idMarker);
    if (it == mMarkers.end())
    {
        MarkerData markerData{ "", idMarker, (currentContext ? currentContext->mMarkerId : -1) };
        it = mMarkers.insert(std::make_pair(idMarker, markerData)).first;
    }

    mContextStack.push(&it->second);
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
void yaget::metrics::Gather::PopParent(IdMarker idMarker)
{
    YAGET_ASSERT(mMarkers.find(idMarker) != mMarkers.end(), "MarkerId: '%d' is not in markers.", idMarker);
    YAGET_ASSERT(!mContextStack.empty() && mContextStack.top()->mMarkerId == idMarker, "MarkerId: '%d' is not at the top of context stack.", idMarker);

    mContextStack.pop();
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
void yaget::metrics::Gather::Reset()
{
    std::stack<const MarkerData*>().swap(mContextStack);
    mMarkers.clear();
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
yaget::metrics::Gather::MarkersTree yaget::metrics::Gather::GetResults() const
{
    return mMarkers;
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
void yaget::metrics::Gather::Activate(bool activate)
{
    internal::gatherer.mActive = activate;
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
const yaget::metrics::Gather& yaget::metrics::Gather::Get()
{
    return yaget::metrics::internal::gatherer;
}
