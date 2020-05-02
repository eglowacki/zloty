#include "MessageInterface.h"
#include "Logger/Log.h"
#include "Message/Dispatcher.h"
#include <boost/bind.hpp>


namespace eg {

Dispatcher::Dispatcher()
: mChannelList(new ChannelList_t)
{
    log_trace(tr_util) << "Dispatcher object created.";
}


Dispatcher::~Dispatcher()
{
    log_trace(tr_util) << "Dispatcher object deleted.";
}


Dispatcher::Channel_t& Dispatcher::operator[](guid_t id)
{
    size_t numChannels = mChannelList->size();
    ChannelList_t::iterator it = mChannelList->find(id);
    if (it == mChannelList->end())
    {
        // let's add new slot here
        mChannelList->insert(std::make_pair(id, new Channel_t()));
        log_trace(tr_util) << "Added new Channel Id: " << logs::hex<guid_t>(id) << " and Total Active Channels is " << numChannels + 1;
    }

    return *((*mChannelList)[id]);
}


/*
void Dispatcher::Post(guid_t mId, boost::any anyData)
{
    boost::mutex::scoped_lock queueLocker(mMutexMessageQueue);
    // \note I'm not sure I need this here anymore, since to actually check for
    // same message I must look into boost::any value
    // let's check to see if this message is already in a queue
    // and if it is, skip adding this new message
//  MessageData_t check(mId, anyData);
//  for (std::vector<MessageData_t>::const_iterator it = mQueuedMessages.begin(); it != mQueuedMessages.end(); ++it)
//  {
//      if ((*it).first == mId)
//      {
//          wxLogWarning("Calling Dispatcher Post message: 0x%X but this message was already posted.", mId);
//          //return;
//      }
//  }

    mQueuedMessages.push_back(std::make_pair(mId, anyData));
}
*/


//void Dispatcher::onTick(Message& /*msg*/)
//{
//    // grab local copy of messages
//    std::vector<MessageData_t> queuedMessages;
//    {
//        boost::mutex::scoped_lock queueLocker(mMutexMessageQueue);
//        queuedMessages = mQueuedMessages;
//        mQueuedMessages.clear();
//    }
//
//    // send all queued messages
//    for (std::vector<MessageData_t>::const_iterator it = queuedMessages.begin(); it != queuedMessages.end(); ++it)
//    {
//        Message msg((*it).first, (*it).second);
//        (*this)[(*it).first](msg);
//    }
//}


//void Dispatcher::onShutdown(Message& /*msg*/)
//{
//    wxLogTrace(tr_util, "Disapatcher kShutdownEnd.");
//
//    mChannelList.reset();
//    boost::mutex::scoped_lock queueLocker(mMutexMessageQueue);
//    mQueuedMessages.clear();
//}


//void Message::Send()
//{
//    if (Dispatcher *disp = registrate::p_cast<Dispatcher>("Dispatcher"))
//    {
//        (*disp)[mType](*this);
//    }
//}


//void Message::Post()
//{
//    if (Dispatcher *disp = registrate::p_cast<Dispatcher>("Dispatcher"))
//    {
//        disp->Post(mType, mAnyData);
//    }
//}

} // namespace eg

