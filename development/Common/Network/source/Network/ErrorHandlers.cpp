#include "Network\ErrorHandlers.h"
#include "Exception/Exception.h"
#include "Platform/Support.h"

#include <boost/system/error_code.hpp>
#include <fmt/format.h>


//---------------------------------------------------------------------------------------------------------------------
void yaget::network::ThrowOnError(const boost::system::error_code& ec, const std::string& message, const std::source_location& location)
{
    if (ec)
    {
        const auto& userMessage = !message.empty() ? fmt::format("{}. ", message) : "";
        auto textError = fmt::format("{}Platform error: {}", userMessage, ec.message());
        if (platform::IsDebuggerAttached())
        {
            YLOG_PERROR("UTIL", location.file_name(), location.line(), location.function_name(), textError.c_str());
            platform::DebuggerBreak();
        }

        textError += fmt::format("\n{}({}) {}", (location.file_name() ? location.file_name() : "no_file"), location.line(), location.function_name());
        throw ex::bad_init(textError);
    }
}
