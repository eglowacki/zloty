#include "Synchronization/CriticalSection.h"
#if 0


namespace eg {

//! pimple pattern, so we do not expose windows related data
class CSData
{
public:
    CSData()
    {
        InitializeCriticalSection(&mCriticalSection);
    }

    ~CSData()
    {
        DeleteCriticalSection(&mCriticalSection);
    }

    bool Try()
    {
        return TryEnterCriticalSection(&mCriticalSection) ? true : false;
    }

    void Lock()
    {
        EnterCriticalSection(&mCriticalSection);
    }

    void Unlock()
    {
        LeaveCriticalSection(&mCriticalSection);
    }

private:
    CRITICAL_SECTION mCriticalSection;
};


CriticalSection::CriticalSection() :
    mCSData(new CSData)
{
}


CriticalSection::~CriticalSection()
{
    delete mCSData;
}


bool CriticalSection::Try()
{
    return mCSData->Try();
}


void CriticalSection::Lock()
{
    mCSData->Lock();
}


void CriticalSection::Unlock()
{
    mCSData->Unlock();
}


} // namespace eg
#endif 0

