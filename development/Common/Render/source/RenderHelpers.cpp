#include "RenderHelpers.h"
#include "Components/LineComponent.h"
#include "Exception/Exception.h"
#include "StringHelpers.h"
#include "Fmt/format.h"
#include "Logger/YLog.h"
#include "Debugging/DevConfiguration.h"
#include "RenderMathFacade.h"
#include <comdef.h>

using namespace yaget;

namespace DirectX::SimpleMath
{
    //const Vector2 Vector2::Zero = { 0.f, 0.f };
    //const Vector2 Vector2::One = { 1.f, 1.f };
    //const Vector2 Vector2::UnitX = { 1.f, 0.f };
    //const Vector2 Vector2::UnitY = { 0.f, 1.f };

    //const Vector3 Vector3::Zero = { 0.f, 0.f, 0.f };
    //const Vector3 Vector3::One = { 1.f, 1.f, 1.f };
    //const Vector3 Vector3::UnitX = { 1.f, 0.f, 0.f };
    //const Vector3 Vector3::UnitY = { 0.f, 1.f, 0.f };
    //const Vector3 Vector3::UnitZ = { 0.f, 0.f, 1.f };
    //const Vector3 Vector3::Up = { 0.f, 1.f, 0.f };
    //const Vector3 Vector3::Down = { 0.f, -1.f, 0.f };
    //const Vector3 Vector3::Right = { 1.f, 0.f, 0.f };
    //const Vector3 Vector3::Left = { -1.f, 0.f, 0.f };
    //const Vector3 Vector3::Forward = { 0.f, 0.f, -1.f };
    //const Vector3 Vector3::Backward = { 0.f, 0.f, 1.f };

    //const Vector4 Vector4::Zero = { 0.f, 0.f, 0.f, 0.f };
    //const Vector4 Vector4::One = { 1.f, 1.f, 1.f, 1.f };
    //const Vector4 Vector4::UnitX = { 1.f, 0.f, 0.f, 0.f };
    //const Vector4 Vector4::UnitY = { 0.f, 1.f, 0.f, 0.f };
    //const Vector4 Vector4::UnitZ = { 0.f, 0.f, 1.f, 0.f };
    //const Vector4 Vector4::UnitW = { 0.f, 0.f, 0.f, 1.f };

    const Matrix Matrix::Identity = { 1.f, 0.f, 0.f, 0.f,
                                      0.f, 1.f, 0.f, 0.f,
                                      0.f, 0.f, 1.f, 0.f,
                                      0.f, 0.f, 0.f, 1.f };

    //const Quaternion Quaternion::Identity = { 0.f, 0.f, 0.f, 1.f };
}


void yaget::render::SetDebugName(ID3D11DeviceChild* d3dData, const std::string& message)
{
    d3dData->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr);

    std::string yagetMessage = "y." + message;
    d3dData->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(yagetMessage.length()), yagetMessage.c_str());
}

void yaget::render::SetDebugName(ID3D11Device* d3dData, const std::string& message)
{
    d3dData->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr);

    std::string yagetMessage = "y." + message;
    d3dData->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(yagetMessage.length()), yagetMessage.c_str());
}

void render::draw::BoundingBox(const math::Box& box, render::LineComponent* lineComponent)
{
    render::LineComponent::LinesPtr_t lines = std::make_shared<render::LineComponent::Lines_t>();
    render::LineComponent::Line line{};
    // P0 -> P1
    line.p0 = math3d::Vector3(box.mMin.x, box.mMin.y, box.mMin.z);
    line.p1 = math3d::Vector3(box.mMax.x, box.mMin.y, box.mMin.z);
    line.c0 = DirectX::Colors::Red;
    line.c1 = DirectX::Colors::Red;
    lines->push_back(line);

    // P1 -> P2
    line.p0 = math3d::Vector3(box.mMax.x, box.mMin.y, box.mMin.z);
    line.p1 = math3d::Vector3(box.mMax.x, box.mMax.y, box.mMin.z);
    line.c0 = DirectX::Colors::Green;
    line.c1 = DirectX::Colors::Green;
    lines->push_back(line);

    // P2 -> P3
    line.p0 = math3d::Vector3(box.mMax.x, box.mMax.y, box.mMin.z);
    line.p1 = math3d::Vector3(box.mMin.x, box.mMax.y, box.mMin.z);
    line.c0 = DirectX::Colors::Red;
    line.c1 = DirectX::Colors::Red;
    lines->push_back(line);

    // P3 -> P0
    line.p0 = math3d::Vector3(box.mMin.x, box.mMax.y, box.mMin.z);
    line.p1 = math3d::Vector3(box.mMin.x, box.mMin.y, box.mMin.z);
    line.c0 = DirectX::Colors::Green;
    line.c1 = DirectX::Colors::Green;
    lines->push_back(line);

    // P4 -> P5
    line.p0 = math3d::Vector3(box.mMin.x, box.mMin.y, box.mMax.z);
    line.p1 = math3d::Vector3(box.mMax.x, box.mMin.y, box.mMax.z);
    line.c0 = DirectX::Colors::Red;
    line.c1 = DirectX::Colors::Red;
    lines->push_back(line);

    // P5 -> P6
    line.p0 = math3d::Vector3(box.mMax.x, box.mMin.y, box.mMax.z);
    line.p1 = math3d::Vector3(box.mMax.x, box.mMax.y, box.mMax.z);
    line.c0 = DirectX::Colors::Green;
    line.c1 = DirectX::Colors::Green;
    lines->push_back(line);

    // P6 -> P7
    line.p0 = math3d::Vector3(box.mMax.x, box.mMax.y, box.mMax.z);
    line.p1 = math3d::Vector3(box.mMin.x, box.mMax.y, box.mMax.z);
    line.c0 = DirectX::Colors::Red;
    line.c1 = DirectX::Colors::Red;
    lines->push_back(line);

    // P7 -> P4
    line.p0 = math3d::Vector3(box.mMin.x, box.mMax.y, box.mMax.z);
    line.p1 = math3d::Vector3(box.mMin.x, box.mMin.y, box.mMax.z);
    line.c0 = DirectX::Colors::Green;
    line.c1 = DirectX::Colors::Green;
    lines->push_back(line);

    // P0 -> P4
    line.p0 = math3d::Vector3(box.mMin.x, box.mMin.y, box.mMin.z);
    line.p1 = math3d::Vector3(box.mMin.x, box.mMin.y, box.mMax.z);
    line.c0 = DirectX::Colors::Blue;
    line.c1 = DirectX::Colors::Blue;
    lines->push_back(line);

    // P1 -> P5
    line.p0 = math3d::Vector3(box.mMax.x, box.mMin.y, box.mMin.z);
    line.p1 = math3d::Vector3(box.mMax.x, box.mMin.y, box.mMax.z);
    lines->push_back(line);

    // P2 -> P6
    line.p0 = math3d::Vector3(box.mMax.x, box.mMax.y, box.mMin.z);
    line.p1 = math3d::Vector3(box.mMax.x, box.mMax.y, box.mMax.z);
    lines->push_back(line);

    // P3 -> P7
    line.p0 = math3d::Vector3(box.mMin.x, box.mMax.y, box.mMin.z);
    line.p1 = math3d::Vector3(box.mMin.x, box.mMax.y, box.mMax.z);
    lines->push_back(line);

    lineComponent->Draw(lines);
}