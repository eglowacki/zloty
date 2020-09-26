#include <Components/RenderComponent.h>
#include "Device.h"
#include "Scene.h"

//--------------------------------------------------------------------------------------------------
yaget::render::RenderComponent::RenderComponent(comp::Id_t id, render::Device& device, Init initOptions, const io::Tags& tags)
    : comp::Component(id)
    , mDevice(device)
    , mValid(false)
    , mResetActivated(false)
    , mChangedTags(initOptions == Init::AutoReset ? true : false)
    , mDeviceSignature(device.GetSignature())
{
    YAGET_ASSERT(dev::CurrentThreadIds().IsThreadRender(), "Creation of component must be called from RENDER thread.");

    AttachAsset(tags);
}
       
//--------------------------------------------------------------------------------------------------
yaget::render::RenderComponent::~RenderComponent()
{
}

//--------------------------------------------------------------------------------------------------
void yaget::render::RenderComponent::Render(const RenderTarget* renderTarget, const math3d::Matrix& matrix)
{
    YAGET_ASSERT(!mResetActivated, "Need to investigate why mResetActivated is true.");

    if ((mValid || mChangedTags) && !mResetActivated)
    {
        if (mDeviceSignature != mDevice.GetSignature() || mChangedTags)
        {
            Reset();
        }

        onRender(renderTarget, matrix);
    }
}

//--------------------------------------------------------------------------------------------------
void yaget::render::RenderComponent::Render(const RenderBuffer& renderBuffer, const DirectX::SimpleMath::Matrix& viewMatrix, const DirectX::SimpleMath::Matrix& projectionMatrix, const RenderComponent::RenderOptions* renderOptions)
{ 
    YAGET_ASSERT(!mResetActivated, "Need to investigate why mResetActivated is true.");
    if ((mValid || mChangedTags) && !mResetActivated)
    {
        if (mDeviceSignature != mDevice.GetSignature() || mChangedTags)
        {
            Reset();
        }

        OnRender(renderBuffer, viewMatrix, projectionMatrix);

        if (renderOptions && renderOptions->bShowGUI)
        {
            OnGuiRender(renderBuffer, viewMatrix, projectionMatrix);
        }
    }
}

//--------------------------------------------------------------------------------------------------
void yaget::render::RenderComponent::Reset()
{
    //! NOTE: do we need a way to check if we Render call is happening?. 
    //! Right now we assume that that Reset and Render will be called from the same render thread
    mResetActivated = true;
    mDeviceSignature = mDevice.GetSignature();
    mChangedTags = false;

    OnReset();
    TriggerSignal(RenderComponent::SignalReset);

    mResetActivated = false;
    mValid = true;
}

//--------------------------------------------------------------------------------------------------
void yaget::render::RenderComponent::AttachAsset(const std::vector<io::Tag>& tags)
{
    std::vector<io::Tag> currentTags = mTags;

    io::Tags validTags;
    std::copy_if(tags.begin(), tags.end(), std::back_inserter(validTags), [](const auto& tag)
    {
        return tag.IsValid();
    });

    bool hasElements = !validTags.empty();
    bool sameSize = validTags.size() == currentTags.size();
    bool isEqual = hasElements && sameSize && std::equal(validTags.begin(), validTags.end(), currentTags.begin());

    bool isDifferent = !(hasElements && (sameSize && isEqual));
    YLOG_CERROR("REND", isDifferent, "Incomming tags and current tags are the same set (duplicate).");

    if (isDifferent)
    {
        mTags = validTags;
        mChangedTags = true;
        mStateHashDirty = true;
    }
}
