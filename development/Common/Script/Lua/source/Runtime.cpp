#include "Scripting/Runtime.h"
#include "Scripting/ResolvedAssets.h"
#include "VTS/ResolvedAssets.h"
#include "App/AppUtilities.h"

#include <pybind11/embed.h> // everything needed for embedding
#include <pybind11/stl.h>
#include <PyLogHook.h>
namespace py = pybind11;


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
    void PythonStdOut(const char* text)
    {
        if (text && std::string(text) != "\n")
        {
            YLOG_DEBUG("PYTH", text);
        }
    }

    void PythonStdErr(const char* text)
    {
        YLOG_ERROR("PYTH", text);
    }

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
                py::module::import("sys").attr("pycache_prefix") = util::ExpendEnv("$(Temp)/PythonByteCode", nullptr);

                YLOG_DEBUG("SCRT", "Module Search Path: '[%s]'. Byte cache folder: '%s'.", 
                    conv::Combine(py::cast<Strings>(py::module::import("sys").attr("path").cast<py::list>()), "], [").c_str(), 
                    py::module::import("sys").attr("pycache_prefix").cast<std::string>().c_str());

                tyti::pylog::redirect_stdout(PythonStdOut);
                tyti::pylog::redirect_stderr(PythonStdErr);
            }
            catch (const py::error_already_set& e)
            {
                std::string errorMessage = e.what();
                YLOG_ERROR("SCRT", "Could not initialize Python environment. %s.", errorMessage.c_str());
            }
        }

        ~PythonScopedContext() = default;

    private:
        py::scoped_interpreter mGuard{};
    };

} // namespace


yaget::scripting::ContextHandle yaget::scripting::CreateContext(const char* niceName)
{
    ContextHandle context = std::make_unique<PythonScopedContext>(niceName);

    return std::move(context);
}


PYBIND11_EMBEDDED_MODULE(syaget, m)
{
}

