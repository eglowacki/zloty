//////////////////////////////////////////////////////////////////////
// TextComponent.h
//
//  Copyright 4/27/2026 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Used to show various text on the screen
//
//  #include "Components/TextComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Component.h"


namespace yaget::comp
{
    class TextComponent : public BaseComponent<DefaultPoolSize>
    {
    public:
        TextComponent(Id_t id)
            : BaseComponent(id)
        {}
    };

} // namespace yaget::comp
