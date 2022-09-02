/////////////////////////////////////////////////////////////////////////
// RenderCore.h
//
//  Copyright 07/12/2022 Edgar Glowacki.
//
// NOTES:
//  Default includes and bits for render library
//      
//
// #include "Render/RenderCore.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "YagetCore.h"
#include <wrl/client.h>

namespace yaget::render
{
    // Alias ComPtr to our render namespace
    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    // Used with *_ptr<> to provide custom deleter (calling Release() method).
    // 
    // Ex:
    // using MyPtr = std::unique_ptr<object_with_release_method, yaget::render::Deleter<object_with_release_method>>;
    template <typename T>
    struct Deleter
    {
        void operator()(T* object) const;
    };

    template <typename T>
    using unique_obj = std::unique_ptr<T, yaget::render::Deleter<T>>;
}


template <typename T>
inline void yaget::render::Deleter<T>::operator()(T* object) const
{
    object->Release();
}
