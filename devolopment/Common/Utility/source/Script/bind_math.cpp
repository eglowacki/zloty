/////////////////////////////////////////////////////////////////////////
// bind_math.cpp
//
//  Copyright 4/8/2009 Edgar Glowacki.
//
// NOTES:
//      Provides bindings for math from lua
//
//
//
/////////////////////////////////////////////////////////////////////////
//! \file
#include "Math/Matrix.h"
#include "Logger/Log.h"
#include "Script/luacpp.h"
#pragma warning(push)
#pragma warning (disable : 4100)  // '' : unreferenced formal parameter
    #include <luabind/luabind.hpp>
#pragma warning(pop)
#include <luabind/operator.hpp>


namespace
{
}


namespace eg { namespace script {

void init_math(lua_State *L)
{
    using namespace luabind;

    module(L, "math")
    [
        class_<Vector3>("Vector3")
            .def(constructor<>())
            .def(constructor<float, float, float>())
            .def(constructor<const Vector3&>())
            .def(tostring(self))
            .def("set", (void(Vector3::*)(float, float, float))&Vector3::set)
            .def("set", (void(Vector3::*)(const Vector3&))&Vector3::set)
            .def_readwrite("x", &Vector3::x)
            .def_readwrite("y", &Vector3::y)
            .def_readwrite("z", &Vector3::z),

        class_<Vector4>("Vector4")
            .def(constructor<>())
            .def(constructor<float, float, float, float>())
            .def(constructor<const Vector4&>())
            .def(tostring(self))
            .def("set", (void(Vector4::*)(float, float, float, float))&Vector4::set)
            .def_readwrite("x", &Vector4::x)
            .def_readwrite("y", &Vector4::y)
            .def_readwrite("z", &Vector4::z)
            .def_readwrite("w", &Vector4::w),

        class_<Matrix44>("Matrix44")
            .def(constructor<>())
            .def(tostring(self)),

        def("PerspectiveMatrix", &PerspectiveMatrixLH44)
    ];
}


}} // namespace script // namespace eg



