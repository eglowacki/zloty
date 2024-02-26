#include "Resources/FontResource.h"
#include "Device.h"
#include "Exception/Exception.h"
#include <SpriteFont.h>

#include "Core/ErrorHandlers.h"


yaget::render::FontResource::FontResource(Device& device, std::shared_ptr<io::render::FontAsset> asset)
    : ResourceView(device, asset->mTag, std::type_index(typeid(FontResource)))
{
    try
    {
        mFont = std::make_unique<DirectX::SpriteFont>(device.GetDevice(), asset->mBuffer.first.get(), asset->mBuffer.second);

        std::size_t hashValue = asset->mTag.Hash();
        SetHashValue(hashValue);
    }
    catch (const std::exception& e)
    {
        const auto& textError = fmt::format("Did not initialize font: '{}'. Error: {}", asset->mTag.ResolveVTS(), e.what());
        error_handlers::Throw("REND", textError);
    }
}

yaget::render::FontResource::~FontResource()
{
}

bool yaget::render::FontResource::Activate()
{
    return false;
}

bool yaget::render::FontResource::Activate(DirectX::SpriteBatch* spriteBatch, const std::string& text, float posX, float posY, const math3d::Color& color)
{
    mDevice.ActivatedResource(this, fmt::format("This: {}, Hash: {}", static_cast<void*>(this), GetStateHash()).c_str());

    mFont->DrawString(spriteBatch, conv::utf8_to_wide(text).c_str(), math3d::Vector2(posX + 1, posY + 1), colors::DarkGray);
    mFont->DrawString(spriteBatch, conv::utf8_to_wide(text).c_str(), math3d::Vector2(posX, posY), color);
    return true;
}

math3d::Vector2 yaget::render::FontResource::MeasureString(const std::string& text) const
{
    auto textSize = mFont->MeasureString(conv::utf8_to_wide(text).c_str());
    math3d::Vector2 convertedSize;
    DirectX::XMStoreFloat2(&convertedSize, textSize);
    return convertedSize;
}
