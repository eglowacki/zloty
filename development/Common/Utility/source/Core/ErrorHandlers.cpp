#include "Core/ErrorHandlers.h"

#include "Exception/Exception.h"
#include "Logger/YLog.h"
#include "Platform/Support.h"

#include "fmt/format.h"
#include <comdef.h>

//---------------------------------------------------------------------------------------------------------------------
void yaget::error_handlers::Throw(const char* tag, const std::string& message, const std::source_location& location)
{
    auto textError = !message.empty() ? fmt::format("{}. ", message) : "";

    if (platform::IsDebuggerAttached())
    {
        const char* logTag =  tag ? tag : "CORE";
        YLOG_PERROR(logTag, location.file_name(), location.line(), location.function_name(), textError.c_str());
        platform::DebuggerBreak();
    }

    textError = fmt::format("{}\n{}({}) {}", textError, location.file_name(), location.line(), location.function_name());
    throw ex::bad_init(textError);
}


//---------------------------------------------------------------------------------------------------------------------
void yaget::error_handlers::ThrowOnError(bool resultValid, const std::string& message, const std::source_location& location)
{
    if (!resultValid)
    {
        const uint64_t hr = ::GetLastError();
        ThrowOnError(static_cast<long>(hr), message, location);
    }
}


//---------------------------------------------------------------------------------------------------------------------
void yaget::error_handlers::ThrowOnError(long hr, const std::string& message, const std::source_location& location)
{
    if (FAILED(hr))
    {
        _com_error cr(HRESULT_FROM_WIN32(hr));
        const char* platformErrorMessage = cr.ErrorMessage();
        const auto textError = fmt::format("{}. HRESULT: {:#x}, Platform error: {}", message, static_cast<unsigned long>(hr), platformErrorMessage);

        Throw(textError, location);
    }
}
