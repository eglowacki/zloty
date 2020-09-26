#include "CameraCalc.h"
#include "Time/GameClock.h"
#include <algorithm>

using namespace yaget;

namespace
{
    math3d::Matrix MakeViewMatrix(const math3d::Vector3& startPos, float pitch, float yaw, float roll, math3d::Vector3& up, math3d::Vector3& look, math3d::Vector3& right)
    {
        /* Start with our camera axis pointing down z
        An alternative method is to just keep adjusting our axis but if we do that the
        axis can start to lose its orthogonal shape (due to floating point innacuracies).
        This could be solved by rebuilding the orthogonal shape each time with the following:
        1. normalising the look vector
        2. creating the up vector from the cross product of the look and the right
        3. normalising up
        4. creating the right vector from the cross product of the look and the up
        5. normalising right
        */

        up = math3d::Vector3(0.0f, 1.0f, 0.0f);
        look = math3d::Vector3(0.0f, 0.0f, 1.0f);
        right = math3d::Vector3(1.0f, 0.0f, 0.0f);

        // Yaw is rotation around the y axis (m_up)
        // Create a matrix that can carry out this rotation
        math3d::Matrix yawMatrix = math3d::Matrix::CreateFromAxisAngle(up, yaw);
        // To apply yaw we rotate the m_look & m_right vectors about the m_up vector (using our yaw matrix)
        look = math3d::Vector3::Transform(look, yawMatrix);
        right = math3d::Vector3::Transform(right, yawMatrix);

        // Pitch is rotation around the x axis (m_right)
        // Create a matrix that can carry out this rotation
        math3d::Matrix pitchMatrix = math3d::Matrix::CreateFromAxisAngle(right, pitch);
        // To apply pitch we rotate the m_look and m_up vectors about the m_right vector (using our pitch matrix)
        look = math3d::Vector3::Transform(look, pitchMatrix);
        up = math3d::Vector3::Transform(up, pitchMatrix);

        // Roll is rotation around the z axis (m_look)
        // Create a matrix that can carry out this rotation
        math3d::Matrix rollMatrix = math3d::Matrix::CreateFromAxisAngle(look, roll);
        // To apply roll we rotate up and right about the look vector (using our roll matrix)
        // Note: roll only really applies for things like aircraft unless you are implementing lean
        right = math3d::Vector3::Transform(right, rollMatrix);
        up = math3d::Vector3::Transform(up, rollMatrix);

        // Build the view matrix from the transformed camera axis
        math3d::Matrix viewMatrix;

        viewMatrix._11 = right.x; viewMatrix._12 = right.y; viewMatrix._13 = right.z;
        viewMatrix._21 = up.x;    viewMatrix._22 = up.y;    viewMatrix._23 = up.z;
        viewMatrix._31 = look.x;  viewMatrix._32 = look.y;  viewMatrix._33 = look.z;

        viewMatrix._41 = -startPos.Dot(right);
        viewMatrix._42 = -startPos.Dot(up);
        viewMatrix._43 = -startPos.Dot(look);

        return viewMatrix;
    }
} // namespace

math3d::Matrix render::CalculateViewMatrix(const math3d::Vector3& startPos, float pitch, float yaw, float roll)
{
    math3d::Vector3 up, look, right;
    return MakeViewMatrix(startPos, pitch, yaw, roll, up, look, right);
}


render::Camera::Camera(const math3d::Vector3& startPos, float pitch, float yaw, float roll)
    : m_position(startPos)
    , m_yaw(yaw)
    , m_pitch(pitch)
    , m_roll(roll)
    , m_up(math3d::Vector3(0.0f, 1.0f, 0.0f))
    , m_look(math3d::Vector3(0.0f, 0.0f, 1.0f))
    , m_right(math3d::Vector3(1.0f, 0.0f, 0.0f))
{
    CalculateViewMatrix();
}

render::Camera::~Camera()
{
}

math3d::Matrix render::Camera::CalculateViewMatrix()
{
    return MakeViewMatrix(m_position, m_pitch, m_yaw, m_roll, m_up, m_look, m_right);
}

void render::Camera::Tick(const time::GameClock& gameClock)
{
    float deltaTime = time::FromTo<float>(gameClock.GetDeltaTime(), time::kMicrosecondUnit, time::kSecondUnit);

    if (mUpdateLastTimestamp)
    {
        mUpdateLastTimestamp = false;
        mLastTimestamp = 32;
    }

    if (mLastTimestamp == 0)
    {
        mInputValues.yaw = 0.0f;
        mInputValues.pitch = 0.0f;
        mInputValues.roll = 0.0f;
    }

    MoveForward(mInputValues.positionLook * deltaTime * mSpeedFactor);
    MoveRight(mInputValues.positionRight * deltaTime * mSpeedFactor);
    MoveUp(mInputValues.positionUp * deltaTime * mSpeedFactor);

    const float kScaler = 5.0f;
    Yaw(mInputValues.yaw * deltaTime * (100.0f / kScaler));
    Pitch(mInputValues.pitch * deltaTime * (100.0f / kScaler));
    Roll(mInputValues.roll * deltaTime * (100.0f / kScaler));

    mLastTimestamp -= time::FromTo<int64_t>(gameClock.GetDeltaTime(), time::kMicrosecondUnit, time::kMilisecondUnit);
    mLastTimestamp = std::max<int64_t>(mLastTimestamp, 0);
}

void render::Camera::Yaw(float amount)
{
    m_yaw += amount;
    m_yaw = RestrictAngleTo360Range(m_yaw);
}

void render::Camera::Pitch(float amount)
{
    m_pitch += amount;
    m_pitch = RestrictAngleTo360Range(m_pitch);
}

void render::Camera::Roll(float amount)
{
    m_roll += amount;
    m_roll = RestrictAngleTo360Range(m_roll);
}

float render::Camera::RestrictAngleTo360Range(float angle) const
{
    while (angle > 2 * DirectX::XM_PI)
    {
        angle -= 2 * DirectX::XM_PI;
    }

    while (angle < 0)
    {
        angle += 2 * DirectX::XM_PI;
    }

    return angle;
}
