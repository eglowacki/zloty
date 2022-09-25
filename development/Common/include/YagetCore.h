///////////////////////////////////////////////////////////////////////
// YagetCore.h
//
//  Copyright 8/13/2017 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Base includes needed to use YagetCore library
//
//
//  #include "YagetCore.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Base.h"
#include "Logger/YLog.h"
#include <vector>


namespace yaget
{
    namespace args { class Options; }
    namespace system
    {
        enum class InitializationResult
        {
            OK,         // all initialization ok
            Helped,     // user asked for help and it was displayed
            ParseError, // there is an options parsing error
            InitError   // initialization error occurred, most likely we can not go further
        };

        // Call this to initialize basic options (you can add yours first) and then call Initialize(...)
        // It will call platform::DisplayDialog(...) to show option and/or init error if options are not correct
        // NOTE: We need a way to turn platform::DisplayDialog
        // data pointer to memory for json data for configuration
        // size is for data, if it's not nullptr
        InitializationResult InitializeSetup(const char* commandLine, args::Options& options, const char* configData, size_t configSize);
        InitializationResult InitializeSetup(int argc, char* argv[], args::Options& options, const char* configData, size_t configSize);
        InitializationResult InitializeSetup(args::Options& options, const char* configData, size_t configSize);
        InitializationResult InitializeSetup(const char* configData = nullptr, size_t configSize = 0, bool skipOptions = false);

        //! Call this to initialize dev, runtime and logging sub-systems. This will set engine state to valid.
        //! Only call this if you need fine control over options and custom error handling,
        // otherwise prefer InitializeSetup variants  
        void Initialize(const args::Options& options, const char* configData, size_t configSize);

    } // namespace yaget

    constexpr uint32_t InvalidId = static_cast<uint32_t>(-1);

    // disable copy on classes
    template <typename T>
    class Noncopyable
    {
    protected:
        Noncopyable(const Noncopyable&) = delete;
        T& operator=(const T&) = delete;
    
        Noncopyable() = default;
        ~Noncopyable() = default;
    };

    template <typename T>
    class NonCopyMove : public Noncopyable<T>
    {
    protected:
        NonCopyMove(NonCopyMove&& other) = delete;
        T& operator=(T&& other) = delete;

        NonCopyMove() = default;
        ~NonCopyMove() = default;
    };


    //// Notes about move
    //class Foo;
    //class Move : public Noncopyable<Foo> // optional if no copy allowed
    //{
    //public:
    //    Move(Move&& other)
    //        : mAFoo(other.mAFoo)
    //    {
    //        other.mAFoo = nullptr;
    //    }

    //    Move& operator=(Move&& other)
    //    {
    //        if (this != &other)
    //        {
    //            mAFoo = other.mAFoo;
    //            other.mAFoo = nullptr;
    //        }

    //        return *this;
    //    }
    //private:
    //    Foo* mAFoo;
    //};

    using Strings = std::vector<std::string>;

    namespace dev
    {
        static const char* DataMarker = "yaget-dev_data.marker";
        static const char* AppDataMarker = "yaget-dev_app_data.marker";
        static const char* UserDataMarker = "yaget-dev_user_data.marker";
    } // namespace dev
} // namespace yaget

// In some case compiler will generate this warning, (Internet has enough explanation about it)
// Adding this macro to cpp file, which generated this warning will suppress it (fixes).
// Parameter x is any valid C++/variable name
#define DISREGARD_LINKER_4221(x) __declspec(dllexport) void YAGET_UNIQUE_NAME(x)() {}



// helper defines during development to suppress any warnings, since we compile with warnings as errors
//
//  YAGET_COMPILE_SUPPRESS_START(4100, "unreferenced local variable")
//  your code here
//  YAGET_COMPILE_SUPPRESS_END
#define YAGET_COMPILE_SUPPRESS_START(x, m) \
__pragma(warning(push)) \
__pragma(warning(disable : x)) \
__pragma(message(__FILE__ "(" YAGET_STRINGIZE(__LINE__) "): [yaget] Disabling Compiler Warning: [" YAGET_STRINGIZE(x) "] - " m))

#define YAGET_COMPILE_SUPPRESS_END __pragma(warning(pop))


// helper defines during development to change warning level around block of code
//
//  YAGET_COMPILE_WARNING_LEVEL_START(3, "Development of new class")
//  your code here
//  YAGET_COMPILE_WARNING_LEVEL_END
#define YAGET_COMPILE_WARNING_LEVEL_START(x, m) \
__pragma(warning(push, x)) \
__pragma(message(__FILE__ "(" YAGET_STRINGIZE(__LINE__) "): [yaget] Changed Warning Level to: [" YAGET_STRINGIZE(x) "] - " m))

#define YAGET_COMPILE_WARNING_LEVEL_END YAGET_COMPILE_SUPPRESS_END

#define YAGET_COMPILE_GLOBAL_SETTINGS(m) \
__pragma(message(__FILE__ "(" YAGET_STRINGIZE(__LINE__) "): [yaget] GLOBAL-SETTING ======== " m " ========"))

// Support for custom functions to provide brand name of product and extra log tags
#define YAGET_GET_BRAND_FUNCTION_NAME GetBrandName
#define YAGET_GET_BRAND_FUNCTION_STRING YAGET_STRINGIZE(YAGET_GET_BRAND_FUNCTION_NAME)
typedef const char* (*YagetFuncBrandName) ();

// It exposes GetBrandName function in application and return user specified brand/company name, which is used in $(Brand) environment alias
#define YAGET_BRAND_NAME_F(name)  extern "C" __declspec(dllexport) const char* YAGET_GET_BRAND_FUNCTION_NAME() { return name; }


// Used in generating schema for GameDirector to provide customization of which c++ keywords need to be filtered out
#define YAGET_USER_STRIP_KEYWORDS_FUNCTION_NAME UserStripKeywords
#define YAGET_USER_STRIP_KEYWORDS_FUNCTION_STRING YAGET_STRINGIZE(YAGET_USER_STRIP_KEYWORDS_FUNCTION_NAME)
typedef const char* (*YagetFuncUserStripKeywords) (const char*);

#define YAGET_USER_STRIP_KEYWORDS_F(name) extern "C" __declspec(dllexport) const char* YAGET_USER_STRIP_KEYWORDS_FUNCTION_NAME(const char* name)

// Helper define to simplify adding user strip keywords
// YAGET_CUSTOMIZE_STRIP_KEYWORDS(",::ttt,ttt::")
#define YAGET_CUSTOMIZE_STRIP_KEYWORDS(set)     \
    YAGET_USER_STRIP_KEYWORDS_F(defaultSet)     \
    {                                           \
        using namespace yaget;                  \
                                                \
        static Initer initer(defaultSet, set);  \
        return initer.mKeywords.c_str();        \
    }

namespace yaget
{
    //! This is used in definition of YAGET_USER_STRIP_KEYWORDS_F to simplify adding of new keywords
    //YAGET_USER_STRIP_KEYWORDS_F(defaultSet)
    //{
    //  static yaget::Initer initer(defaultSet, ",::ttt,ttt::");
    //  return initer.mKeywords.c_str();
    //}
    //
    // or use alternate macro
    // YAGET_CUSTOMIZE_STRIP_KEYWORDS(",::ttt,ttt::")

    struct Initer
    {
        Initer(const char* defaults, const char* customKeywords)
            : mKeywords(defaults ? std::string(defaults) + customKeywords : customKeywords)
        {}

        std::string mKeywords;
    };

}

// get size of struct at compile time
// define YAGET_GET_STRUCT_SIZE before calling ANY headers (do that in your cpp at the top of the file)
// and then call yaget::meta::print_size_at_compile and compile will error out with error message,
// which will contain size of the type you have passed.
// Example:
//      #define YAGET_GET_STRUCT_SIZE
//      const int holder = yaget::meta::print_size_at_compile<yaget::metrics::TraceRecord>();
//
// If YAGET_GET_STRUCT_SIZE is not define, print_size_at_compile does nothing
// Output (using TraceRecord struct with 80 bytes in size as an example):
// 1>Meta\CompilerAlgo.h(467,1): error C2440: 'initializing': cannot convert from 'int' to 'char (*)[80]'
// 1>PerformanceTracer.cpp(4): message : see reference to function template instantiation 'int yaget::meta::print_size_at_compile<TraceRecord>(void)' being compiled
