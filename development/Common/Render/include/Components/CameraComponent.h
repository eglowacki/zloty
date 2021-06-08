/////////////////////////////////////////////////////////////////////////
// CameraComponent.h
//
//  Copyright 8/03/2016 Edgar Glowacki.
//
// NOTES:
//      Provides camera implementation (first pass)
//
//
// #include "Components/CameraComponent.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/RenderComponent.h"
#include "RenderMathFacade.h"
#include "CameraCalc.h"


namespace
{
    class InputActionToggle;
    class InputActionDrag;
}

namespace yaget
{
    namespace comp
    {
        class PhysicsComponent;
    } // namespace comp

    namespace render
    {
        class CameraComponent : public RenderComponent
        {
        public:
            CameraComponent(comp::Id_t id, Device& device, comp::PhysicsComponent* physics);
            virtual ~CameraComponent();

            void Tick(const time::GameClock& gameClock) override;

            void SetView(const math3d::Vector3& startPos, float pitch, float yaw, float roll);

        private:
            size_t CalculateStateHash() const override { YAGET_ASSERT(false, "Implemented CalculateStateHash methos!!!"); return 0; }

            void OnReset() override;
            void OnRender(const RenderBuffer& renderBuffer, const DirectX::SimpleMath::Matrix& viewMatrix, const DirectX::SimpleMath::Matrix& projectionMatrix) override;
            void OnGuiRender(const RenderBuffer& renderBuffer, const DirectX::SimpleMath::Matrix& viewMatrix, const DirectX::SimpleMath::Matrix& projectionMatrix) override;

            void OnAction(const std::string& actionName, uint64_t timeStamp, int32_t mouseX, int32_t mouseY, uint32_t flags);

            comp::PhysicsComponent* mPhysics = nullptr;
            DirectX::SimpleMath::Vector3 mActionDirection;

            std::unique_ptr<InputActionToggle> mVFToggle;
            std::unique_ptr<InputActionToggle> mVBToggle;
            std::unique_ptr<InputActionToggle> mVLToggle;
            std::unique_ptr<InputActionToggle> mVRToggle;
            std::unique_ptr<InputActionDrag> mViewRot;

            std::unique_ptr<InputActionToggle> mVRLeftToggle;
            std::unique_ptr<InputActionToggle> mVRRightToggle;
            std::unique_ptr<InputActionToggle> mVRUpToggle;
            std::unique_ptr<InputActionToggle> mVRDownToggle;

            int mLastMouseX = 0;
            Camera mCameraCalc;

            // used in input for mouse delta control
            bool mViewControlActive = false;
        };

    } // namespace render
} // namespace yaget
