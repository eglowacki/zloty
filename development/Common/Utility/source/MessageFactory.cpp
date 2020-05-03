#if 0
#include "Network/MessageFactory.h"
#include "INetwork.h"
#include "MessageInterface.h"
#include "Message/Dispatcher.h"
#include <wx/log.h>
#include <boost/bind.hpp>


namespace eg {
namespace network {


MessageFactory::MessageFactory()
: mNetworkState(esEmpty)
{
    Dispatcher& disp = REGISTRATE(Dispatcher);
    disp[mtype::kNetworkConnectionState].connect(boost::bind(&MessageFactory::onNetworkState, this, _1));
}


bool MessageFactory::Process(Packet *pPacket, uint32_t systemId, int type, NetworkMeesageCallback_t messageCallback)
{
    HandlerKey_t key(systemId, type);
    MessageHndlers_t::iterator it = mHandlers.find(key);
    if (it != mHandlers.end())
    {
        return (*it).second->Process(pPacket, messageCallback);
    }

    wxLogWarning("No Message Handler for type: %d, SystemId: %x", key.second, key.first);
    return false;
}


bool MessageFactory::Process(Packet *pPacket, uint32_t systemId, int type)
{
    MessageCallbacks_t::const_iterator it = mMessageCallbacks.find(systemId);
    if (it != mMessageCallbacks.end())
    {
        return Process(pPacket, systemId, type, (*it).second);
    }

    return Process(pPacket, systemId, type, 0);
}


void MessageFactory::addMessageHandler(MessageHandler *pMessageHandler, uint32_t systemId, int type)
{
    HandlerKey_t key(systemId, type);
    MessageHndlers_t::const_iterator it = mHandlers.find(key);
    if (it == mHandlers.end())
    {
        wxLogTrace(tr_util, "Registering Message Handler for type: %d, SystemId: %x", key.second, key.first);
        mHandlers.insert(std::make_pair(key, pMessageHandler));
    }
    else
    {
        wxLogWarning("Message Handler already exist for type: %d, SystemId: %x", key.second, key.first);
    }
}


void MessageFactory::removeMessageHandler(uint32_t systemId, int type)
{
    HandlerKey_t key(systemId, type);
    MessageHndlers_t::iterator it = mHandlers.find(key);
    if (it != mHandlers.end())
    {
        wxLogTrace(tr_util, "Removing Message Handler for type: %d, SystemId: %x", key.second, key.first);
        mHandlers.erase(it);
    }
    else
    {
        wxLogWarning("Trying to remove Message Handler which does not exist for type: %d, SystemId: %x", key.second, key.first);
    }
}



MessageHandler *MessageFactory::FindHandler(uint32_t systemId, int type) const
{
    HandlerKey_t key(systemId, type);
    MessageHndlers_t::const_iterator it = mHandlers.find(key);
    if (it != mHandlers.end())
    {
        return (*it).second;
    }

    return 0;
}


void MessageFactory::SetMessageCallabck(uint32_t systemId, NetworkMeesageCallback_t messageCallback)
{
    MessageCallbacks_t::const_iterator it = mMessageCallbacks.find(systemId);
    if (it == mMessageCallbacks.end())
    {
        wxLogTrace(tr_util, "Seeting Message Handler Callback for SystemId: %x", systemId);
        mMessageCallbacks.insert(std::make_pair(systemId, messageCallback));
    }
    else if (messageCallback.empty())
    {
        wxLogTrace(tr_util, "Removing Message Handler Callback for SystemId: %x", systemId);
        mMessageCallbacks.erase(systemId);
    }
    else
    {
        wxLogWarning("Message Handler Callback already exist for SystemId: %x", systemId);
    }
}


void MessageFactory::onNetworkState(const Message& msg)
{
    if (msg.Is<uint32_t>())
    {
        mNetworkState = msg.GetValue<uint32_t>();
        wxLogTrace(tr_util, "Network State changed to: %d", mNetworkState);
    }
}


uint32_t MessageFactory::GetState() const
{
    return mNetworkState;
}


} // namespace network
} // namespace eg

#endif // 0
