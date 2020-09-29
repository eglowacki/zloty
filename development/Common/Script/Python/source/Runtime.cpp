#include "Scripting/Runtime.h"
#include "App/AppUtilities.h"
#include "Components/LocationComponent.h"
#include "Meta/CompilerAlgo.h"
#include "Scripting/PythonComponent.h"
#include "Scripting/ResolvedAssets.h"
#include "Time/GameClock.h"
#include "VTS/ResolvedAssets.h"

#include <PyLogHook.h>
#include <pybind11/cast.h>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

#include <filesystem>


namespace py = pybind11;
namespace fs = std::filesystem;


//Py_SetProgramName(conv::utf8_to_wide("db_test_and_python").c_str());  /* optional but recommended */
//Py_Initialize();

//PyRun_SimpleString("import sys\nprint(sys.path)\n");
//auto ret = Py_FinalizeEx();
//ret = ret;
//void print_dict(py::dict dict) 
//{
//    /* Easily interact with Python types */
//    for (auto item : dict)
//    {

//        std::cout << "key=" << std::string(py::str(item.first)) << ", "
//        << "value=" << std::string(py::str(item.second)) << std::endl;
//    }
//}
//{
//    py::scoped_interpreter guard{}; // start the interpreter and keep it alive

//    YLOG_NOTICE("SCRT", "Started Python version '%s', using pybind11 version '%d.%d'.", PY_VERSION, PYBIND11_VERSION_MAJOR, PYBIND11_VERSION_MINOR);

//    py::module::import("sys").attr("path").cast<py::list>().append("c:/Development/yaget/Data");
//    py::module::import("sys").attr("pycache_prefix") = util::ExpendEnv("$(Temp)/PythonByteCode", nullptr);


//    std::map<std::string, py::handle> gs = py::globals().cast<std::map<std::string, py::handle>>();

//    YLOG_DEBUG("SCRT", "Module Search Path: '[%s]'.", conv::Combine(py::cast<Strings>(py::module::import("sys").attr("path").cast<py::list>()), "], [").c_str());

//    try
//    {
//        py::exec(R"(
//            import sys
//            print(sys.path)
//        )");

//        py::module calc = py::module::import("calc");
//        py::object result = calc.attr("add")(1, 2);
//        int n = result.cast<int>();

//        //gs = calc.attr["__dict__"].cast<std::map<std::string, py::handle>>();
//        //auto t = py::str(calc).cast<std::string>();

//        gs = py::globals().cast<std::map<std::string, py::handle>>();
//        int z = 0;
//        z;

//    }
//    catch (const py::error_already_set& e)
//    {
//        std::string errorMessage = e.what();
//        auto ex = e;

//        int z = 0;
//        z;
//    }
//}
//py::list list;
//for (auto item : list)
//{
//    if (py::isinstance<py::str>(item))
//    {
//        // do stuff
//    }
//}
//
//py::cast(cpp_obj);
//py_obj.cast<T>();

// example of type conversion for function f between c++ and python
//m.def("for_even", [](int n, std::function<void(int)> f)
//{
//    for (int i = 0; i < n; ++i)
//    {
//        if (i % 2 == 0)
//        {
//            f(i);
//        }
//    }
//});
//
//>>> def callback(x):
//...     print('received:', x)
//
//>>> for_even(3, callback)
//received: 0
//received: 1


namespace
{
    // returns info about python script current file, line and function/<module> from where the print(...)/output was called in .py file
    // NOTE: what about if we load files data from memory?
    using FileDebugLink = std::tuple<std::string /*file_name*/, int /*line_no*/, std::string /*function_name*/>;

    FileDebugLink FormatCurrentFileDebugLink()
    {
        if (PyFrameObject* pyFrame = PyEval_GetFrame())
        {
            int lineNumber = PyFrame_GetLineNumber(pyFrame);
            std::string functionName = pyFrame->f_code ? py::str(pyFrame->f_code->co_name) : "";

            std::string fileName;
            auto gs = py::globals().cast<std::map<std::string, py::handle>>();
            if (const auto it = gs.find("__file__"); it != std::end(gs))
            {
                fileName = it->second.cast<std::string>();
            }

            return std::make_tuple(fileName, lineNumber, functionName);
        }

        return {};
    }

    // used in redirecting any std::out to our DEBUG log (same for one below but uses err and ERROR log
    // NOTE: what about Log Levels in C++, do we want to replicate this
    void PythonStdOut(const char* text)
    {
        if (text && std::string(text) != "\n")
        {
            auto debugLink = FormatCurrentFileDebugLink();

            YLOG_PDEBUG("PYTH", std::get<0>(debugLink).c_str(), std::get<1>(debugLink), std::get<2>(debugLink).c_str(), text);
        }
    }

    void PythonStdErr(const char* text)
    {
        if (text && std::string(text) != "\n")
        {
            auto debugLink = FormatCurrentFileDebugLink();

            YLOG_PERROR("PYTH", std::get<0>(debugLink).c_str(), std::get<1>(debugLink), std::get<2>(debugLink).c_str(), text);
        }
    }

    // Starts up Python VM and keeps it alive for duration of this object lifetime
    class PythonScopedContext : public yaget::scripting::Context
    {
        using Context = yaget::scripting::Context;

    public:
        PythonScopedContext(const char* niceName) : Context(niceName)
        {
            using namespace yaget;

            YLOG_NOTICE("SCRT", "Started Python version '%s', using pybind11 version '%d.%d'.", PY_VERSION, PYBIND11_VERSION_MAJOR, PYBIND11_VERSION_MINOR);

            try
            {
                // we do not want to populate folder with chached py files, so we re-direct them to temp/*
                py::module::import("sys").attr("pycache_prefix") = util::ExpendEnv("$(Temp)/PythonByteCode", nullptr);

                YLOG_DEBUG("SCRT", "Module Search Path: '[%s]'. Byte cache folder: '%s'.", 
                    conv::Combine(py::cast<Strings>(py::module::import("sys").attr("path").cast<py::list>()), "], [").c_str(), 
                    py::module::import("sys").attr("pycache_prefix").cast<std::string>().c_str());

                // re-direct any out and err from python to our own c++ systems. This allows us to use YLOG functionality for any outputs
                tyti::pylog::redirect_stdout(PythonStdOut);
                tyti::pylog::redirect_stderr(PythonStdErr);

                mModule = py::module::import("syaget");
            }
            catch (const py::error_already_set& e)
            {
                const std::string errorMessage = e.what();
                YLOG_ERROR("SCRT", "Could not initialize Python environment. %s.", errorMessage.c_str());
            }
        }

        pybind11::module& Module() override
        {
            return mModule;
        }

    private:
        [[maybe_unused]] py::scoped_interpreter mGuard{};
        py::module mModule;
    };

    enum class Classifier
    {
        Placeholder,
        Overview,
        BuiltInMethod,
        Class,
        Function,
        Module,
        Other
    };

    const yaget::Strings ClassifierText =
    {
        {"Placeholder"},
        {"Overview"},
        {"Built in methods"},
        {"Classes"},
        {"Functions"},
        {"Modules"},
        {"Other"},
    };

    Classifier GetFromString(const std::string& textLine)
    {
        if (textLine.find("<built-in method ") != std::string::npos)
        {
            return Classifier::BuiltInMethod;
        }
        else if (textLine.find("<class ") != std::string::npos)
        {
            return Classifier::Class;
        }
        else if (textLine.find("<function ") != std::string::npos)
        {
            return Classifier::Function;
        }
        else if (textLine.find("<module ") != std::string::npos)
        {
            return Classifier::Module;
        }

        return Classifier::Other;
    }

} // namespace


yaget::scripting::ContextHandle yaget::scripting::CreateContext(const char* niceName)
{
    ContextHandle context = std::make_unique<PythonScopedContext>(niceName);

    return std::move(context);
}


bool yaget::scripting::ExecuteInitialize(Context* context, const Executor& executor)
{
    using namespace yaget;

    try
    {
        auto& m = context->Module();
        executor(m);
        return true;
    }
    catch (const py::error_already_set& e)
    {
        std::string errorMessage = e.what();
        YLOG_ERROR("SCRT", "Could not Execute Initializers: '%s'. ex type: {%s}", e.what(), meta::type_name<decltype(e)>().data());
    }
    catch (const py::builtin_exception& e)
    {
        std::string errorMessage = e.what();
        YLOG_ERROR("SCRT", "Could not Execute Initializers: '%s'. ex type: {%s}", e.what(), meta::type_name<decltype(e)>().data());
    }
    catch (const std::exception& e)
    {
        std::string errorMessage = e.what();
        YLOG_ERROR("SCRT", "Could not Execute Initializers: '%s'. ex type: {%s}", e.what(), meta::type_name<decltype(e)>().data());
    }

    return false;
}


std::string yaget::scripting::PrintHelp(pybind11::module& pyModule)
{
        using ClassifierKey = std::pair<Classifier, std::string>;
        // used to sort sections within attributes list
        struct comparator
        {
            bool operator()(const ClassifierKey& a, const ClassifierKey& b) const
            {
                if (a.first != b.first)
                {
                    return a.first < b.first;
                }
                else if (a.first == b.first)
                {
                    const bool aSpecial = a.second.starts_with("__") && a.second.ends_with("__");
                    const bool bSpecial = b.second.starts_with("__") && b.second.ends_with("__");

                    if (aSpecial && bSpecial)
                    {
                        return a.second < b.second;
                    }
                    if (!aSpecial && bSpecial)
                    {
                        return false;
                    }
                    if (aSpecial && !bSpecial)
                    {
                        return true;
                    }
                }

                return a < b;
            }
        };
        using Variables = std::map<ClassifierKey, std::string, comparator>;

        Variables variables;

        const std::set<std::string>& exclude = { "__builtins__", "__cached__", "__loader__", "__package__", "__spec__" };

        auto dict = py::getattr(pyModule, "__dict__");
        auto ls = dict.cast<std::map<std::string, py::handle>>();
        for (const auto& [key, value] : ls)
        {
            if (exclude.find(key) != std::end(exclude))
            {
                // we matched exclusion for this key, skip it and go to next one
                continue;
            }

            auto varType = py::str(value).cast<std::string>();
            auto classif = GetFromString(varType);
            if (key.starts_with("__") && key.ends_with("__"))
            {
                classif = Classifier::Overview;
            }

            auto doc = classif == Classifier::Overview ? std::string{} : py::str(value.doc()).cast<std::string>();
            if (!doc.empty() && *doc.rbegin() == '\n')
            {
                doc.erase(doc.size() - 1);
            }

            auto it = std::unique(doc.begin(), doc.end());
            doc.resize(std::distance(doc.begin(), it));
            if (doc.empty() || doc == "None")
            {
                doc = varType;
            }
            else
            {
                doc = varType + "\n" + doc;
            }

            yaget::conv::ReplaceAll(doc, "\n", "\n\t");
            variables[std::make_pair(classif, key)] = doc;
        }

        Classifier classifier = Classifier::Placeholder;

        std::string message;
        const size_t MaxHeaderLen = 100;
        for (const auto& [key, value] : variables)
        {
            if (classifier != key.first)
            {
                classifier = key.first;
                const std::string headerText = "----------------- " + ClassifierText[static_cast<size_t>(key.first)] + " ";
                message += headerText;

                const auto leftLen = MaxHeaderLen - headerText.size();
                message += std::string(leftLen, '-') + "\n";
            }
            if (classifier == Classifier::Overview && key.second == "__file__")
            {
                auto filePath = fs::path(value).make_preferred().string();
                message += filePath + "\n";
            }
            else
            {
                message += key.second + " - " + value + "\n";
            }
        }

        if (!message.empty() && *message.rbegin() == '\n')
        {
            message.erase(message.size() - 1);
        }

        return message;
}


PYBIND11_EMBEDDED_MODULE(syaget, m)
{
    using namespace yaget;

    py::class_<math3d::Vector3>(m, "Vector3")
        .def(py::init<float, float, float>())
        .def_static("Transform", static_cast<math3d::Vector3(*)(const math3d::Vector3&, const  math3d::Quaternion&)>(&math3d::Vector3::Transform))
        ;

    py::class_<math3d::Quaternion>(m, "Quaternion")
        .def_static("CreateFromAxisAngle", static_cast<math3d::Quaternion(*)(const math3d::Vector3& /*axis*/, float /*angle*/)>(&math3d::Quaternion::CreateFromAxisAngle));
        ;

    py::class_<time::GameClock>(m, "GameClock", "Exposes game time, current frame counter and delta time. All timing values are in microseconds.")
        .def_property_readonly("TickCounter", [](const time::GameClock& gameClock) { return gameClock.GetLogicTime(); }, "Return current frame counter, monotonic increase every frame.")
        .def_property_readonly("LogicTime", [](const time::GameClock& gameClock) { return gameClock.GetLogicTime(); }, "Return current logic/game time. Time stamp is take at the beginning of a frame. This is not a wall clock.")
        .def_property_readonly("DeltaTime", [](const time::GameClock& gameClock) { return gameClock.GetDeltaTime(); }, "Last frame delta time.")
        ;

    py::class_<comp::Component>(m, "Component", "Base class representing game specific components. This provides an id and when it was created time stamped.")
        .def("BeginLife", &comp::Component::BeginLife, "Returns time of creation of this component.")
        .def("Id", &comp::Component::Id, "Returns id of this component.")
        ;

    py::class_<comp::LocationComponent, comp::Component>(m, "LocationComponent", "Represents location in a game world.")
        .def("SetScale", &comp::LocationComponent::SetScale)
        .def("SetOrientation", &comp::LocationComponent::SetOrientation)
        .def("SetPosition", &comp::LocationComponent::SetPosition)
        ;
    
    py::class_<scripting::PythonComponent, comp::Component> pc(m, "PythonComponent", "Manages loading, initialization and running a script associated with this component")
        ;

    m.def("MicroToSeconds", [](time::Microsecond_t value)
    {
        return time::FromTo<float>(value, time::kMicrosecondUnit, time::kSecondUnit);
    }, "Convert time units from microseconds to seconds");

    m.def("SecondsToMicro", [](float value)
    {
        return time::FromTo<time::Microsecond_t>(value, time::kSecondUnit, time::kMicrosecondUnit);
    }, "Convert time units from seconds to microseconds");

    m.def("MilliToMicro", [](time:: Milisecond_t value)
    {
        return time::FromTo<time::Microsecond_t>(value, time::kMilisecondUnit, time::kMicrosecondUnit);
    }, "Convert time units from milliseconds to microseconds");

    m.def("MicroToMilli", [](time::Microsecond_t value)
    {
        return time::FromTo<time::Milisecond_t>(value, time::kMicrosecondUnit, time::kMilisecondUnit);
    }, "Convert time units from microseconds to milliseconds");

    m.def("RadToDeg", &math3d::RadToDeg, "Translate radians to degrees");
    m.def("DegToRad", &math3d::DegToRad, "Translate degrees to radians");
}
