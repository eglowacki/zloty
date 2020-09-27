#include "Components/CameraComponent.h"
#include "Components/PhysicsComponent.h"
#include "Device.h"
#include "App/Application.h"
#include "Input/InputDevice.h"
#include "Debugging/Assert.h"
#include "StringHelpers.h"
#include "Scene.h"
#include <imgui.h>

#include <algorithm>

using namespace yaget;
using namespace DirectX;

namespace
{
    template<typename T, typename U>
    T clamp(T in, U low, U high)
    {
        if (in <= low)
        {
            return low;
        }
        else if (in >= high)
        {
            return high;
        }
        else
        {
            return in;
        }
    }

    bool IsActionSame(const std::string& actionName, const std::string& baseName)
    {
        if (!baseName.empty())
        {
            return actionName.compare(0, baseName.size(), baseName) == 0;
        }
        return true;
    }

    bool IsActionPlusType(const std::string& actionName, const std::string& baseName, const std::string& plusType)
    {
        size_t len = plusType.size();
        if (len)
        {
            bool bResult = actionName.compare(actionName.size() - len, len, plusType) == 0;
            if (bResult)
            {
                bResult = IsActionSame(actionName, baseName);
            }
            return bResult;
        }
        else
        {
            return actionName.compare(0, baseName.size(), baseName) == 0;
        }
    }

    const char* Up = "+Up";
    const char* Down = "+Down";
    const char* Drag = "+Drag";

    class InputActionToggle
    {
    public:
        InputActionToggle(const std::string& actionName, input::ActionCallback_t actionCallback, input::InputDevice& input)
            : mActionName(actionName)
            , mInput(input)
            , mActionCallback(actionCallback)
        {
            YAGET_ASSERT(actionCallback, "actionCallback for InputActionToggle is nullptr.");
            using namespace std::placeholders;
            auto fun = [this](auto&&... params) { OnAction(params...); };
            mInput.RegisterActionCallback(actionName, fun);
        }

        ~InputActionToggle()
        {}

    private:
        void OnAction(const std::string& actionName, uint64_t timeStamp, int32_t mouseX, int32_t mouseY, uint32_t flags)
        {
            YAGET_ASSERT(mActionName == actionName, "InputActionToggle mismatch between action names. this: '%s', param: '%s'", mActionName.c_str(), actionName.c_str());
            if (flags & input::kButtonUp)
            {
                mActionCallback(mActionName + Up, timeStamp, mouseX, mouseY, flags);
            }
            else if (flags & input::kButtonDown)
            {
                mActionCallback(mActionName + Down, timeStamp, mouseX, mouseY, flags);
            }
        }

        std::string mActionName;
        input::InputDevice& mInput;
        input::ActionCallback_t mActionCallback;
    };

    //uint32_t
    class InputActionDrag
    {
    public:
        InputActionDrag(const std::string& actionName, input::ActionCallback_t actionCallback, input::InputDevice& input)
            : mActionName(actionName)
            , mInput(input)
            , mActionCallback(actionCallback)
        {
            YAGET_ASSERT(actionCallback, "actionCallback for InputActionDrag is nullptr.");
            using namespace std::placeholders;
            auto fun = [this](auto&&... params) { OnAction(params...); };
            mInput.RegisterActionCallback(actionName, fun);
        }

        ~InputActionDrag()
        {}

    private:
        void OnAction(const std::string& actionName, uint64_t timeStamp, int32_t mouseX, int32_t mouseY, uint32_t flags)
        {
            YAGET_ASSERT(mActionName == actionName, "InputActionDrag mismatch between action names. this: '%s', param: '%s'", mActionName.c_str(), actionName.c_str());

            std::string actionPostfix = actionName;
            if (flags & input::kButtonDown)
            {
                mDragging = true;
                actionPostfix += Down;
            }
            else if (flags & input::kButtonUp)
            {
                mDragging = false;
                actionPostfix += Up;
            }
            else if (flags & input::kMouseMove && mDragging)
            {
                actionPostfix += Drag;
            }

            if (actionName != actionPostfix)
            {
                mActionCallback(actionPostfix, timeStamp, mouseX, mouseY, flags);
            }
        }

        std::string mActionName;
        input::InputDevice& mInput;
        input::ActionCallback_t mActionCallback;
        bool mDragging = false;
    };


    const float kCameraMovementSpeed = 0.8f;
    const float kCameraRotationSpeed = 0.05f;

    void GetRotation(const math3d::Matrix& matrix, float& Yaw, float& Pitch, float& Roll)
    {
        if (matrix(1, 1) == 1.0f)
        {
            Yaw = atan2f(matrix(1, 3), matrix(3, 4));
            Pitch = 0;
            Roll = 0;

        }
        else if (matrix(1, 1) == -1.0f)
        {
            Yaw = atan2f(matrix(1, 3), matrix(3,4));
            Pitch = 0;
            Roll = 0;
        }
        else
        {
            Yaw = atan2(-matrix(3, 1), matrix(1, 1));
            Pitch = asin(matrix(2, 1));
            Roll = atan2(-matrix(2, 3), matrix(2, 2));
        }
    }
} // namespace

yaget::render::CameraComponent::CameraComponent(comp::Id_t id, Device& device, comp::PhysicsComponent* physics)
    : RenderComponent(id, device, Init::AutoReset, {})
    , mPhysics(physics)
    , mCameraCalc(physics->GetMatrix().Translation()/*math3d::Vector3(4.5f, 494.0f, 0)*/, math3d::kPI / 2.0f/*1.49f*/, 0.0f, 0.0f)
{
    using namespace std::placeholders;
    auto fun = [this](auto&&... params) { OnAction(params...); };
    input::InputDevice& input = mDevice.App().Input();

    mVFToggle = std::make_unique<InputActionToggle>("View Forward", fun, input);
    mVBToggle = std::make_unique<InputActionToggle>("View Back", fun, input);
    mVLToggle = std::make_unique<InputActionToggle>("View Left", fun, input);
    mVRToggle = std::make_unique<InputActionToggle>("View Right", fun, input);

    mVRLeftToggle = std::make_unique<InputActionToggle>("View Rotate Left", fun, input);
    mVRRightToggle = std::make_unique<InputActionToggle>("View Rotate Right", fun, input);
    mVRUpToggle = std::make_unique<InputActionToggle>("View Rotate Up", fun, input);
    mVRDownToggle = std::make_unique<InputActionToggle>("View Rotate Down", fun, input);

    input.RegisterActionCallback("View.Control", [this](const std::string& /*actionName*/, uint64_t /*timeStamp*/, int32_t mouseX, int32_t mouseY, uint32_t /*flags*/)
    {
        //if (mViewControlActive)
        {
            if (mouseX > 0)
            {
                mCameraCalc.Input_Yaw(kCameraRotationSpeed);
            }
            else if (mouseX < 0)
            {
                mCameraCalc.Input_Yaw(-kCameraRotationSpeed);
            }
            else if (mouseX == 0)
            {
                mCameraCalc.Input_Yaw(0);
            }

            if (mouseY > 0)
            {
                mCameraCalc.Input_Pitch(kCameraRotationSpeed);
            }
            else if (mouseY < 0)
            {
                mCameraCalc.Input_Pitch(-kCameraRotationSpeed);
            }
            else if (mouseY == 0)
            {
                mCameraCalc.Input_Pitch(0);
            }
        }
    });
}

render::CameraComponent::~CameraComponent()
{}

void render::CameraComponent::Tick(const time::GameClock& gameClock)
{
    mCameraCalc.Tick(gameClock);
    math3d::Matrix viewMatrix = mCameraCalc.CalculateViewMatrix();
    mPhysics->SetMatrix(viewMatrix);
}

void render::CameraComponent::SetView(const math3d::Vector3& startPos, float pitch, float yaw, float roll)
{
    mCameraCalc = Camera(startPos, pitch, yaw, roll);
    math3d::Matrix viewMatrix = mCameraCalc.CalculateViewMatrix();
    mPhysics->SetMatrix(viewMatrix);
}

void render::CameraComponent::OnRender(const RenderBuffer& /*renderBuffer*/, const DirectX::SimpleMath::Matrix& /*viewMatrix*/, const DirectX::SimpleMath::Matrix& /*projectionMatrix*/)
{
}

void render::CameraComponent::OnGuiRender(const RenderBuffer& /*renderBuffer*/, const DirectX::SimpleMath::Matrix& /*viewMatrix*/, const DirectX::SimpleMath::Matrix& /*projectionMatrix*/)
{
    if (ImGui::Begin("Yaget Debug Options"))
    {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.4f), "Camera:");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.3f), "Loc: %s", conv::Convertor<SimpleMath::Vector3>::ToString(mCameraCalc.GetPosition()).c_str());
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.3f), "Rot: Pitch: %.2f, Yaw: %.2f, Roll: %.2f, ", mCameraCalc.GetPitch(), mCameraCalc.GetYaw(), mCameraCalc.GetRoll());

        ImGui::SliderFloat("Camera Speed:", &mCameraCalc.mSpeedFactor, 0.0f, 100.0);

        //ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.3f), "Rot: %f", RadToDeg(mCameraData.mCurrentRotation));
    }
    ImGui::End();
}

void render::CameraComponent::OnReset()
{}

void render::CameraComponent::OnAction(const std::string& actionName, uint64_t /*timeStamp*/, int32_t /*mouseX*/, int32_t /*mouseY*/, uint32_t /*flags*/)
{
    if (IsActionPlusType(actionName, "View Forward", Down))
    {
        mCameraCalc.Input_MoveForward(kCameraMovementSpeed);
    }
    else if (IsActionPlusType(actionName, "View Back", Down))
    {
        mCameraCalc.Input_MoveForward(-kCameraMovementSpeed);
    }
    else if (IsActionPlusType(actionName, "View Forward", Up) || IsActionPlusType(actionName, "View Back", Up))
    {
        mCameraCalc.Input_MoveForward(0.0f);
    }
    else if (IsActionPlusType(actionName, "View Right", Down))
    {
        mCameraCalc.Input_MoveRight(kCameraMovementSpeed);
    }
    else if (IsActionPlusType(actionName, "View Left", Down))
    {
        mCameraCalc.Input_MoveRight(-kCameraMovementSpeed);
    }
    else if (IsActionPlusType(actionName, "View Right", Up) || IsActionPlusType(actionName, "View Left", Up))
    {
        mCameraCalc.Input_MoveRight(0.0f);
    }
    else if (IsActionPlusType(actionName, "View Rotate Left", Down))
    {
        mCameraCalc.Input_Yaw(-kCameraRotationSpeed);
    }
    else if (IsActionPlusType(actionName, "View Rotate Right", Down))
    {
        mCameraCalc.Input_Yaw(kCameraRotationSpeed);
    }
    else if (IsActionPlusType(actionName, "View Rotate Left", Up) || IsActionPlusType(actionName, "View Rotate Right", Up))
    {
        mCameraCalc.Input_Yaw(0.0f);
    }
    else if (IsActionPlusType(actionName, "View Rotate Up", Down))
    {
        mCameraCalc.Input_Pitch(-kCameraRotationSpeed);

    }
    else if (IsActionPlusType(actionName, "View Rotate Down", Down))
    {
        mCameraCalc.Input_Pitch(kCameraRotationSpeed);
    }
    else if (IsActionPlusType(actionName, "View Rotate Up", Up) || IsActionPlusType(actionName, "View Rotate Down", Up))
    {
        mCameraCalc.Input_Pitch(0.0f);
    }
}

render::CameraComponentPool::Ptr render::CameraComponentPool::New(comp::Id_t id, Device& device, comp::PhysicsComponent* physics)
{
    Ptr c = NewComponent(id, device, physics);
    return c;
}
