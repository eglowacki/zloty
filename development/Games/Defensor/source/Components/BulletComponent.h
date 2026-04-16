
/////////////////////////////////////////////////////////////////////////
// BulletComponent.h
//
//  Copyright 4/14/2026 Edgar Glowacki.
//
// NOTES:
//      
//
//
// #include "Components/BulletComponent.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Components/Component.h"


namespace yaget::comp
{
        class BulletComponent : public BaseComponent<DefaultPoolSize>
    {
    public:
        BulletComponent(Id_t id)
            : BaseComponent(id)
        {}
    };


}
