#include "Synchronization/WaitObject.h"


namespace eg {


WaitObject::WaitObject()
{
}


void WaitObject::Wait(EventObject& eventObject, int32_t waitMiliseconds)
{
    HANDLE handle = static_cast<HANDLE>(eventObject.GetHandle());
    DWORD waitTime = INFINITE;
    if (waitMiliseconds != kWaitInfinite)
    {
        waitTime = waitMiliseconds;
    }

    ::WaitForSingleObject(handle, waitTime);
}


} // namespace eg
