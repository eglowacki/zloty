//////////////////////////////////////////////////////////////////////
// RenderPassComponent.h
//
//  Copyright 11/15/2026 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Used to mark component as render pass, so that RenderSystem can find it and execute it.
//
//
//  #include "Components/RenderPassComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Component.h"


namespace yaget::comp
{
    class RenderPassComponent : public BaseComponent<DefaultPoolSize>
    {
    public:
        RenderPassComponent(Id_t id)
            : BaseComponent(id)
        {}
    };

} // namespace yaget::comp
