//////////////////////////////////////////////////////////////////////
// PongerComponents.h
//
//  Copyright 7/12/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "Ponger/PongerComponents.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "HashUtilities.h"
#include "YagetCore.h"
#include "Components/Component.h"
#include "Streams/Buffers.h"

namespace ponger
{
    class DebugComponent;
}

namespace yaget::comp::db
{
    struct VisualAsset {};

    template <>
    struct ComponentProperties<ponger::DebugComponent>
    {
        using Row = std::tuple<VisualAsset>;
        using Types = std::tuple<io::Tag>;
        static Types DefaultRow() { return Types{}; };
    };
    
}


namespace ponger
{
    using namespace yaget;

    // Placeholder for extra ponger component to denote that we need renderable comp on render side
    class DebugComponent : public comp::Component
    {
    public:
        DebugComponent(comp::Id_t id, io::Tag visualTag) : comp::Component(id), mVisualTag(visualTag)  {}

        const io::Tag& GetVisualTag() const { return mVisualTag; }
        const math3d::Color& GetColor() const { return mColor; }

        void SetVisualTag(const io::Tag& tag);
        void SetColor(const colors::Color& color);

    private:
        size_t CalculateStateHash() const override;

        io::Tag mVisualTag;
        math3d::Color mColor;
    };

} // namespace ponger


namespace std
{
    template <>
    struct hash<ponger::DebugComponent>
    {
        size_t operator()(const ponger::DebugComponent& debugComponent) const
        {
            const math3d::Color& color = debugComponent.GetColor();
            const yaget::Guid& visualTag = debugComponent.GetVisualTag().mGuid;

            return yaget::conv::GenerateHash(color, visualTag);
        }
    };
} // namespace std

