//////////////////////////////////////////////////////////////////////
// Runtime.h
//
//  Copyright 4/18/2020 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      missing_notes
//
//
//  #include "Scripting/Runtime.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include <functional>

namespace pybind11 { class module; }
namespace yaget::args { class Options; }

namespace yaget::scripting
{
    class Context
    {
    public:
        Context(const char* niceName) : mNiceName(niceName ? niceName : "")
        {}
        virtual ~Context() = default;

        virtual pybind11::module& Module() = 0;

    private:
        std::string mNiceName;
    };

    using ContextHandle = std::unique_ptr<Context>;

    ContextHandle CreateContext(const char* niceName);

    using Executor = std::function<void(pybind11::module& pyModule)>;
    bool ExecuteInitialize(Context* context, const Executor& executor);

    // Return formatted string showing all symbols, signature, type and doc
    std::string PrintHelp(pybind11::module& pyModule);
} // namespace yaget::scripting
