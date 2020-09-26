//////////////////////////////////////////////////////////////////////
// FontResource.h
//
//  Copyright 8/17/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "Resources/FontResource.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Resources/ResourceView.h"
#include "MathFacade.h"

namespace DirectX { class SpriteFont; class SpriteBatch; }

namespace yaget::io::render { class FontAsset; }

namespace yaget
{
    namespace io { class JsonAsset; }

    namespace render
    {
        class FontResource : public ResourceView
        {
        public:
            FontResource(Device& device, std::shared_ptr<io::render::FontAsset> asset);

            bool Activate() override;
            bool Activate(DirectX::SpriteBatch* spriteBatch, const std::string& text, float posX, float posY, const math3d::Color& color);
            const char* GetNameType() const override { return "Font"; }

            math3d::Vector2 MeasureString(const std::string& text) const;

        private:
            std::unique_ptr<DirectX::SpriteFont> mFont;
        };

    } // namespace render
} // namespace yaget