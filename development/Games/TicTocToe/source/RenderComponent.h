/////////////////////////////////////////////////////////////////////////
// RenderComponent.h
//
//  Copyright 10/25/2020 Edgar Glowacki.
//
// NOTES:
//      Renders various components
//
// #include "RenderComponent.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Components/Component.h"
#include "Components/ComponentTypes.h"

namespace ttt
{
    class RenderComponent : public yaget::comp::BaseComponent<yaget::comp::DefaultPoolSize>
    {
    public:
        RenderComponent(yaget::comp::Id_t id, yaget::io::Tags tags);

    private:
        yaget::io::Tags mTags;
    };
}
