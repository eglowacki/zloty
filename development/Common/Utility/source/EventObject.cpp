#include "Synchronization/EventObject.h"
#if 0


namespace eg {

class EOData
{
public:
    EOData(bool bManualReset, bool bInitialState) :
        mEvent(::CreateEvent(0, bManualReset, bInitialState, 0))
    {
    }

    ~EOData()
    {
        ::CloseHandle(mEvent);
    }

    void Set()
    {
        ::SetEvent(mEvent);
    }

    void Reset()
    {
        ::ResetEvent(mEvent);
    }

    void Pulse()
    {
        ::PulseEvent(mEvent);
    }

    void *GetHandle() const
    {
        return mEvent;
    }

private:
    HANDLE mEvent;
};


EventObject::EventObject(bool bManualReset, bool bInitialState) :
    mEOData(new EOData(bManualReset, bInitialState))
{
}


EventObject::~EventObject()
{
    delete mEOData;
}


void EventObject::Signal()
{
    mEOData->Set();
}


void EventObject::Reset()
{
    mEOData->Reset();
}


void EventObject::Pulse()
{
    mEOData->Pulse();
}


void *EventObject::GetHandle() const
{
    return mEOData->GetHandle();
}


} // namespace eg
#endif 0

