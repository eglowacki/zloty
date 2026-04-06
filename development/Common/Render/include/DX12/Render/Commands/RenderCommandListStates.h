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

#include <d3dx12.h>

struct ID3D12Device;
struct ID3D12Resource;
struct ID3D12DescriptorHeap;


namespace yaget::render::commands
{
    class CommandList;

    D3D12_RESOURCE_STATES TransitionToRenderTarget(const CommandList* commandList, D3D12_RESOURCE_STATES fromState, ID3D12Resource* renderTarget, ID3D12DescriptorHeap* rtDescriptorHeap, ID3D12DescriptorHeap* dsDescriptorHeap, uint32_t frameIndex);
    void ClearRenderTarget(const CommandList* commandList, const colors::Color& color, ID3D12Resource* renderTarget, ID3D12DescriptorHeap* descriptorHeap, uint32_t frameIndex);
    void ClearDepthStencil(const CommandList* commandList, float depth, uint8_t stencil, ID3D12DescriptorHeap* dsDescriptorHeap);

    // Transition render target from state to state
    D3D12_RESOURCE_STATES TransitionFromTo(const CommandList* commandList, ID3D12Resource* renderTarget, D3D12_RESOURCE_STATES fromState, D3D12_RESOURCE_STATES toState);

}


