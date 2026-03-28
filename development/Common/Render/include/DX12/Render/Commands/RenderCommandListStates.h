/////////////////////////////////////////////////////////////////////////
// RenderCommandListStates.h
//
//  Copyright 03/24/2026 Edgar Glowacki.
//
// NOTES:
//      This handles graphics command list helper functions
//      like Transitions and Clear
//
// #include "Render/Commands/RenderCommandListStates.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Render/RenderCore.h"
#include "MathFacade.h"

struct ID3D12Device;
struct ID3D12Resource;
struct ID3D12DescriptorHeap;


namespace yaget::render::commands
{
    class CommandList;

    void TransitionToRenderTarget(CommandList* commandList, ID3D12Resource* renderTarget, ID3D12DescriptorHeap* descriptorHeap, int frameIndex);
    void TransitionToPresent(const CommandList* commandList, ID3D12Resource* renderTarget);
    void ClearRenderTarget(CommandList* commandList, const colors::Color& color, ID3D12Resource* renderTarget, ID3D12DescriptorHeap* descriptorHeap, int frameIndex);

}


