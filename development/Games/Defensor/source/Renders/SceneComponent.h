///////////////////////////////////////////////////////////////////////
// SceneComponent.h
//
//  Copyright 6/23/2025 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//
//  #include "Renders/SceneComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/Component.h"
#include "DefensorRenderTypes.h"

namespace defensor::render
{
    using namespace yaget;

    class SceneComponent : public comp::BaseComponent<comp::GlobalPoolSize>
    {
    public:
        SceneComponent(comp::Id_t id);

        const render::EntityState* FindState(comp::Id_t id) const;
        comp::ItemIds GetIds() const;

        std::vector<render::EntityState> mEntities;
    };

}
