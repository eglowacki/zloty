/////////////////////////////////////////////////////////////////////////
// TextComponent.h
//
//  Copyright 7/26/2016 Edgar Glowacki.
//
// NOTES:
//      Provides very simple text out capabilities
//
//
// #include "Components/TextComponent.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Components/RenderComponent.h"
#include "Resources/FontResource.h"
#include "Resources/ResourceWatchRefresher.h"

namespace DirectX { class SpriteBatch; }

namespace yaget
{
    namespace render
    {
        class TextComponent : public render::RenderComponent
        {
        public:
            struct TextData
            {
                std::string mFontName;
                std::string mText;
                float mPosX = 0.0f;
                float mPosY = 0.0f;
                math3d::Color mColor;
            };

            using TextBlocks = std::vector<TextData>;

            // Initialize text component with fonts (tags)
            TextComponent(comp::Id_t id, Device& device, const io::Tags& fontTags);

            void ClearText();
            void SetText(const std::string& fontName, const std::string& text, float posX, float posY, const colors::Color& color);
            void SetText(const TextData& textData) { SetText(TextBlocks{ textData }); }
            void SetText(const TextBlocks& textBlocks);

            math3d::Vector2 MeasureString(const std::string& fontName, const std::string& text) const;

        private:
            size_t CalculateStateHash() const override;
            void OnReset() override;
            void onRender(const RenderTarget* renderTarget, const math3d::Matrix& matrix) override;
            void OnRender(const RenderBuffer& /*renderBuffer*/, const DirectX::SimpleMath::Matrix& /*viewMatrix*/, const DirectX::SimpleMath::Matrix& /*projectionMatrix*/) override {}
            void OnGuiRender(const RenderBuffer& /*renderBuffer*/, const DirectX::SimpleMath::Matrix& /*viewMatrix*/, const DirectX::SimpleMath::Matrix& /*projectionMatrix*/) override {}

            using TextRequests = std::vector<TextData>;
            TextRequests mTextRequests;

            std::unique_ptr<DirectX::SpriteBatch> mSpriteBatch;

            using FontWatcherCollection = ResourceWatchCollection<render::FontResource>;
            std::unique_ptr<FontWatcherCollection> mFontCollection;
        };

    } // namespace render
} // namespace yaget
