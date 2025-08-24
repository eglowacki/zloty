//////////////////////////////////////////////////////////////////////
// VelocityComponent.h
//
//  Copyright 8/8/2025 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Integration of input action into velocity and then updating Location
//
//  #include "Components/VelocityComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Component.h"


namespace yaget::comp
{
    class VelocityComponent : public BaseComponent<DefaultPoolSize>
    {
    public:
        VelocityComponent(Id_t id)
            : BaseComponent(id)
        {}
    };

} // namespace yaget::comp
