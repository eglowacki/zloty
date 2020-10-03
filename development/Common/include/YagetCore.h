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
#include "Debugging/Assert.h"
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
        //! Only call this if you need fine control over options and custom error handling, prefer InitializeSetup variant  
        void Initialize(const args::Options& options, const char* configData, size_t configSize);

    } // namespace yaget

    constexpr uint32_t InvalidId = static_cast<uint32_t>(-1);

    // disable copy on classes
    template <typename T>
    class Noncopyable
    {
    public:
        Noncopyable(const Noncopyable&) = delete;
        T& operator=(const T&) = delete;

    protected:
        Noncopyable() = default;
        ~Noncopyable() = default;
    };

    //// Notes about move
    //class Foo;
    //class Move
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


    static const char* DataMarker = "yaget_data.marker";
}

// In some case compiler will generate this warning, (Internet has enough explanation about it)
// Adding this macro to cpp file, which generated this warning will suppress it (fixes).
// Parameter x is any valid C++/variable name
#define DISREGARD_LINKER_4221(x) __declspec(dllexport) void YAGET_UNIQUE_NAME(x)() {}



// helper defines during development to suppress any warnings, since we compile with warnings as errors
//
//  YAGET_COMPILE_SUPRESS_START(4100, "'': unreferenced local variable")
//  YAGET_COMPILE_SUPRESS_END
#define YAGET_COMPILE_SUPRESS_START(x, m) \
__pragma(warning(push)) \
__pragma(warning(disable : x)) \
__pragma(message(__FILE__ "(" YAGET_STRINGIZE(__LINE__) "): [yaget] Disabling Compiler Warning: [" YAGET_STRINGIZE(x) "] - " m))

#define YAGET_COMPILE_SUPRESS_END __pragma(warning(pop))


// It exposes GetBrandName function in application and return user specified brand/company name, which used in $(Brand) alias environment
#define YAGET_BRAND_NAME(name)  extern "C" __declspec(dllexport) const char* GetBrandName() { return name; }
