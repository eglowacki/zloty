#include "Components/Component.h"
#include "Debugging/Assert.h"
#include "Debugging/DevConfiguration.h"
#include "Platform/Support.h"

using namespace yaget;

comp::Component::~Component()
{
    mSignalHeaders.clear();
}

comp::Component::Component(Id_t id) 
    : BaseComponent(id)
{
}

void comp::Component::TriggerSignal(int signalSig)
{
    //std::lock_guard<std::mutex> locker(mHeadersMutex);
    if (IsSignalConnected(signalSig))
    {
        auto it = mSignalHeaders.find({ signalSig, nullptr });
        (*it).mFunction(*this);
    }
}

void comp::Component::AddSupportedTrigger(int signalSig)
{
    // we don't mutex this area, since it should only be called during ctor 
    // TODO: How to enforce calling method in ctor only?
    YAGET_ASSERT(!IsSignalPublished(signalSig), "Trigger: '%d' for '%s' type already exist, can not add multiply times.", signalSig, "");
    mSignalHeaders.emplace(SignalHeader{ signalSig, nullptr });
}

bool comp::Component::IsSignalPublished(int signalSig) const
{
    auto it = mSignalHeaders.find({ signalSig, nullptr });
    return it != mSignalHeaders.end();
}

bool comp::Component::IsSignalConnected(int signalSig) const
{
    auto it = mSignalHeaders.find({ signalSig, nullptr });
    return it != mSignalHeaders.end() && (*it).mFunction;
}

void comp::Component::Tick(const time::GameClock& /*gameClock*/)
{
}

bool comp::Component::ConnectTrigger(int signalSig, comp::Component::TriggerFunction triggerFunction)
{
    //std::lock_guard<std::mutex> locker(mHeadersMutex);
    if (IsSignalPublished(signalSig))
    {
        YAGET_ASSERT(!IsSignalConnected(signalSig), "Signal '%d' for '%s' type already connected", signalSig, "");
        mSignalHeaders.erase({ signalSig, nullptr });
        mSignalHeaders.emplace(SignalHeader{ signalSig, triggerFunction });

        // when new hookup is established, we want to initiate a trigger
        TriggerSignal(signalSig);
        return true;
    }
    else
    {
        // NOTE: should we add log here?
    }

    return false;
}

size_t comp::Component::GetStateHash() const
{
    if (mStateHashDirty)
    {
        mStateHashDirty = false;
        mStateHash = CalculateStateHash();
    }

    return mStateHash;
}

void yaget::comp::Component::UpdateGui(UpdateGuiType updateGuiType)
{
    YAGET_ASSERT(dev::CurrentThreadIds().IsThreadRender(), "UpdateGui of component must be called from RENDER thread.");

    onUpdateGui(updateGuiType);
}
