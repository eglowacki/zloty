/////////////////////////////////////////////////////////////////////////
// CameraCalc.h
//
//  Copyright 2/11/2017 Edgar Glowacki.
//
// NOTES:
//      Provides camera calculations object for various types of
//      control and movement
//
//
// #include "CameraCalc.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "RenderMathFacade.h"


namespace yaget
{
    namespace time { class GameClock; }

    namespace render
    {
        class Camera
        {
        public:
            Camera(const math3d::Vector3& startPos, float pitch, float yaw, float roll);
            ~Camera();

            void Tick(const time::GameClock& gameClock);
            math3d::Matrix CalculateViewMatrix();

            void Input_MoveForward(float amount) { mInputValues.positionLook = amount; }
            void Input_MoveRight(float amount) { mInputValues.positionRight = amount; }
            void Input_MoveUp(float amount) { mInputValues.positionUp = amount; }

            void Input_Yaw(float amount) { mInputValues.yaw = amount; mUpdateLastTimestamp = true;  }
            void Input_Pitch(float amount) { mInputValues.pitch = amount; mUpdateLastTimestamp = true; }
            void Input_Roll(float amount) { mInputValues.roll = amount; mUpdateLastTimestamp = true; }

            // Gets
            float GetYaw() const { return m_yaw; }
            float GetPitch() const { return m_pitch; }
            float GetRoll() const { return m_roll; }
            const math3d::Vector3& GetPosition() const { return m_position; }

            float mSpeedFactor = 100.0f;

        private:
            // Move operations
            void MoveForward(float amount) { m_position += m_look*amount; }
            void MoveRight(float amount) { m_position += m_right*amount; }
            void MoveUp(float amount) { m_position += m_up*amount; }

            // Rotations
            void Yaw(float amount);     // rotate around x axis
            void Pitch(float amount);   // rotate around x axis
            void Roll(float amount);    // rotate around z axis	

            float RestrictAngleTo360Range(float angle) const;

            math3d::Vector3 m_position; // camera position
            float m_yaw;                // rotation around the y axis
            float m_pitch;              // rotation around the x axis
            float m_roll;               // rotation around the z axis
            math3d::Vector3 m_up;
            math3d::Vector3 m_look;
            math3d::Vector3 m_right;

            struct InputValues
            {
                float yaw = 0;
                float pitch = 0;
                float roll = 0;
                float positionLook = 0;
                float positionRight = 0;
                float positionUp = 0;
            };
            InputValues mInputValues;

            int64_t mLastTimestamp = 0;
            bool mUpdateLastTimestamp = false;
        };

        math3d::Matrix CalculateViewMatrix(const math3d::Vector3& startPos, float pitch, float yaw, float roll);

    } // namespace render
} // namespace yaget
