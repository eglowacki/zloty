#include "Logger/Log.h"

#ifdef YAGET_USE_LOGGER
#include "App/AppUtilities.h"
//#include "Registrate.h"

#pragma warning(push)
    #pragma warning (disable : 4244)  // '' : conversion from 'int' to 'unsigned short', possible loss of data
    #pragma warning (disable : 4512)  // '' : assignment operator could not be generated
    #include <boost/logging/format/formatter/tags.hpp>
    #include <boost/logging/format/formatter/high_precision_time.hpp>
    #include <boost/logging/format/destination/file.hpp>
    #include <boost/logging/detail/level.hpp>
    #include <boost/logging/format.hpp>
#pragma warning(pop)

//! own filter implementation to take module name and return true or false
//! if trace for specific module is on
bool FilterLevelModule::is_enabled(bl::level::type level, const char *module_name) const
{
    if (finder_t::filter::is_enabled(level))
    {
        if (module_name)
        {
            std::string module(module_name);
            if (module == "all")
            {
                return true;
            }
            std::vector<std::string>::const_iterator it = std::find(traceModules.begin(), traceModules.end(), module);
            return it != traceModules.end();
        }
    }

    return false;
}


#pragma warning(push)
#pragma warning (disable : 4250)  // '' : inherits 'type' via dominance
struct module_output : bl::formatter::class_<module_output, bl::formatter::implement_op_equal::no_context>
{
    void operator()(param str) const
    {
        //param::arg_type text = str;
        std::string text = str;
        const boost::logging::tag::module& module = str;
        std::string moduleName(module.val);
        if (!moduleName.empty())
        {
            text = "(" + moduleName + ") " + text;
        }
        str.set_string(text);
    }
};
#pragma warning(pop)



BOOST_DEFINE_LOG_FILTER(LoggerFilter, FilterLevelModule)
BOOST_DEFINE_LOG(Logger, Logger_t)



namespace eg {
                                       namespace logs {

void Initialize(const std::string& logName, uint32_t destFlags, int logLevel, bool bActive, const std::vector<std::string>& traceModules)
{
    LoggerFilter()->traceModules = traceModules;
    std::string logFilePath(logName);
    if (logFilePath.empty())
    {
        logFilePath = util::getAppFullPath() + ".log";
    }

    Logger()->writer().add_formatter(module_output());
    //Logger()->writer().add_formatter(bl::formatter::tag::module(), "(%) ");
    Logger()->writer().add_formatter(bl::formatter::tag::level());
    Logger()->writer().add_formatter(bl::formatter::high_precision_time("$hh:$mm.$ss.$mili "));
    Logger()->writer().add_formatter(bl::formatter::thread_id(), "[%] ");
    Logger()->writer().add_formatter(bl::formatter::tag::file_line());
    Logger()->writer().add_formatter(bl::formatter::append_newline());
    Logger()->writer().add_destination(bl::destination::dbg_window());
    Logger()->writer().add_destination(bl::destination::cout());
    Logger()->writer().add_destination(bl::destination::file(logFilePath, bl::destination::file_settings().initial_overwrite(true).do_append(false)));

    // cleat any default destinations
    Logger()->writer().router().set_route().clear();

    if (destFlags & kDest_file)
    {
        Logger()->writer().router().append_route().clear()
            // format for file output:
            // 'hh:mm:ss:mil [level] (module_name) message itself\n'
            .fmt(module_output())
            //.fmt(bl::formatter::spacer(bl::formatter::tag::module(), "(%) "))
            .fmt(bl::formatter::tag::level())
            .fmt(bl::formatter::spacer(bl::formatter::thread_id(), "[%] "))
            .fmt(bl::formatter::high_precision_time("$hh:$mm.$ss.$mili "))
            .fmt(bl::formatter::append_newline())
            .dest(bl::destination::file(logFilePath, bl::destination::file_settings().initial_overwrite(true).do_append(false)));
    }

    if (destFlags & kDest_cout)
    {
        Logger()->writer().router().append_route().clear().clear()
            // output to debug window, where
            // we want to show full file path and line number
            // first, so when user double clicks in Visual Studio,
            // it will take you to that source line
            // format:
            // 'full_file_path(line_number) : hh:mm:ss:mil [level] (module_name) message itself\n'
            .fmt(module_output())
            //.fmt(bl::formatter::spacer(bl::formatter::tag::module(), "(%) "))
            .fmt(bl::formatter::tag::level())
            .fmt(bl::formatter::high_precision_time("$hh:$mm.$ss.$mili "))
            .fmt(bl::formatter::tag::file_line())
            .fmt(bl::formatter::append_newline())
            .dest(bl::destination::cout());
        /*
        Logger()->writer().router().append_route().clear()
            // format for cout:
            // 'hh:mm:ss:mil [level] (module_name) message itself\n'
            .fmt(bl::formatter::spacer(bl::formatter::tag::module(), "(%) "))
            .fmt(bl::formatter::tag::level())
            .fmt(bl::formatter::high_precision_time("$hh:$mm.$ss.$mili "))
            .fmt(bl::formatter::append_newline())
            .dest(bl::destination::cout());
        */
    }

    if (destFlags & kDest_debug)
    {
        Logger()->writer().router().append_route().clear().clear()
            // output to debug window, where
            // we want to show full file path and line number
            // first, so when user double clicks in Visual Studio,
            // it will take you to that source line
            // format:
            // 'full_file_path(line_number) : hh:mm:ss:mil [level] (module_name) message itself\n'
            .fmt(module_output())
            //.fmt(bl::formatter::spacer(bl::formatter::tag::module(), "(%) "))
            .fmt(bl::formatter::tag::level())
            .fmt(bl::formatter::spacer(bl::formatter::thread_id(), "[%] "))
            .fmt(bl::formatter::high_precision_time("$hh:$mm.$ss.$mili "))
            .fmt(bl::formatter::tag::file_line())
            .fmt(bl::formatter::append_newline())
            .dest(bl::destination::dbg_window());
    }

    Logger()->mark_as_initialized();
    if (!bActive)
    {
        LoggerFilter()->set_enabled(false);
    }

    LoggerFilter()->set_enabled(logLevel);

    log_trace(tr_util) << "Initialized log, name: '" << logFilePath << "', destination: " << hex<uint32_t>(destFlags) << ", level: " << logLevel << ", active: " << boolean(bActive);
}


} // namespace logs
} // namespace eg

#endif // YAGET_USE_LOGGER
