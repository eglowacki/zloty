#include "Components/TextComponent.h"
#include "Resources/ResourceWatchRefresher.h"
#include "Device.h"
#include "HashUtilities.h"
#include "App/Application.h"
#include <SpriteBatch.h>
#include <utility>

#include "Core/ErrorHandlers.h"


//---------------------------------------------------------------------------------------------------------------------
yaget::render::TextComponent::TextComponent(comp::Id_t id, Device& device, const io::Tags& fontTags)
    : render::RenderComponent(id, device, Init::Default, fontTags)
{
}


//---------------------------------------------------------------------------------------------------------------------
void yaget::render::TextComponent::OnReset()
{
    mFontCollection = {};
    mt::Variable<io::Tags>::Type tags = mTags;

    mFontCollection = std::make_unique<FontWatcherCollection>(mDevice, tags);

    try
    {
        Device::ID3D11DeviceContext_t* deviceContext = mDevice.GetDeviceContext();
        mSpriteBatch = std::make_unique<DirectX::SpriteBatch>(deviceContext);
    }
    catch (const std::exception& e)
    {
        const auto& textError = fmt::format("did not initialize spritebatch. error: {}", e.what());
        error_handlers::Throw("REND", textError);
    }
}


//---------------------------------------------------------------------------------------------------------------------
void yaget::render::TextComponent::onRender(const RenderTarget* /*renderTarget*/, const math3d::Matrix& /*matrix*/)
{
    mSpriteBatch->Begin();

    for (const auto& textData : mTextRequests)
    {
        mFontCollection->Process(textData.mFontName, [this, &textData](auto font)
        {
            return font->Activate(mSpriteBatch.get(), textData.mText, textData.mPosX, textData.mPosY, textData.mColor);
        });
    }

    mSpriteBatch->End();
}


//---------------------------------------------------------------------------------------------------------------------
void yaget::render::TextComponent::ClearText()
{
    YAGET_ASSERT(dev::CurrentThreadIds().IsThreadRender(), "ClearText must be called from RENDER thread.");

    mTextRequests = {};
}


//---------------------------------------------------------------------------------------------------------------------
void yaget::render::TextComponent::SetText(const TextBlocks& textBlocks)
{
    mTextRequests.insert(std::end(mTextRequests), std::begin(textBlocks), std::end(textBlocks));
}


//---------------------------------------------------------------------------------------------------------------------
void yaget::render::TextComponent::SetText(const std::string& fontName, const std::string& text, float posX, float posY, const colors::Color& color)
{
    YAGET_ASSERT(dev::CurrentThreadIds().IsThreadRender(), "SetText must be called from RENDER thread.");

    mTextRequests.emplace_back(TextData{ fontName, text, posX, posY, color });
}


//---------------------------------------------------------------------------------------------------------------------
math3d::Vector2 yaget::render::TextComponent::MeasureString(const std::string& fontName, const std::string& text) const
{
    math3d::Vector2 textSize;

    if (mFontCollection)
    {
        mFontCollection->Process(fontName, [&textSize, &text](auto font)
        {
            textSize = font->MeasureString(text);
            return true;
        });
    }

    return textSize;
}


//---------------------------------------------------------------------------------------------------------------------
size_t yaget::render::TextComponent::CalculateStateHash() const
{
    mt::Variable<io::Tags>::Type tags = mTags;
    return conv::GenerateHash(tags);
}
