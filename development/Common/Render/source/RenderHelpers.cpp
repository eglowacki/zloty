#include "RenderHelpers.h"
#include "Components/LineComponent.h"
#include "Exception/Exception.h"
#include "StringHelpers.h"
#include "Fmt/format.h"
#include "Logger/YLog.h"
#include "Debugging/DevConfiguration.h"
#include "MathFacade.h"
#include <comdef.h>

using namespace yaget;


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