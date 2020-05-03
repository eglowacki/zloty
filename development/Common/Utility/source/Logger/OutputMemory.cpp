#include "YagetCore.h"
#include "LoggerCpp/Output.h"
#include "LoggerCpp/Config.h"
#include "LoggerCpp/Manager.h"



namespace yaget::ylog
{

    class OutputMemory : public Output
    {
    public:
        explicit OutputMemory(const Config::Ptr& aConfigPtr)
        {

        }

    private:
        void OnOutput(const Channel::Ptr& aChannelPtr, const Log& aLog) const override
        {

        }
    };

} // namespace yaget::ylog

namespace
{
    struct FooBar
    {
        FooBar()
        {
            //yaget::ylog::Manager::RegisterOutputType<yaget::ylog::OutputMemory>(&yaget::ylog::CreateOutputInstance<yaget::ylog::OutputMemory>);
        }
    } fooBar;
} // namespace