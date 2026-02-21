#include "Network/ErrorHandlers.h"
#include "Exception/Exception.h"
#include "Platform/Support.h"

#include <boost/system/error_code.hpp>


//---------------------------------------------------------------------------------------------------------------------
void yaget::error_handlers::ThrowOnError(const boost::system::error_code& ec, const std::string& message, const std::source_location& location)
{
    if (ec)
    {
        const auto& userMessage = !message.empty() ? std::format("{}. ", message) : "";
        const auto textError = std::format("{}{}", userMessage, ec.message());
        Throw(textError, location);
    }
}
