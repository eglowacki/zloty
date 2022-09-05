/////////////////////////////////////////////////////////////////////////
// Commander.h
//
//  Copyright 08/28/2022 Edgar Glowacki.
//
// NOTES:
//      Wrapper for setting command list
//
// #include "Render/Platform/Commander.h"
// 
// https://asawicki.info/news_1719_two_shader_compilers_of_direct3d_12
// Example of using new dx shader compiler
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Render/RenderCore.h"

struct IDXGISwapChain2;
struct ID3D12DescriptorHeap;
struct ID3D12GraphicsCommandList;

namespace colors { struct Color; } 

namespace yaget::render::platform
{
    //-------------------------------------------------------------------------------------------------
    class Commander
    {
    public:
        Commander(uint32_t rtvDescriptorHandleSize, ID3D12DescriptorHeap* descriptorHeap);
        ~Commander();

        void SetRenderTarget(IDXGISwapChain2* swapChain, ID3D12GraphicsCommandList* commandList, uint32_t frameIndex);
        void ClearRenderTarget(const colors::Color& color, ID3D12GraphicsCommandList* commandList, uint32_t frameIndex);

    private:
        const uint32_t mRTVDescriptorHandleSize = 0;
        ID3D12DescriptorHeap* mDescriptorHeap = nullptr;
    };
}