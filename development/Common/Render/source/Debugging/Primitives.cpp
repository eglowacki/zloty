#include "Debugging/Primitives.h"
#include "Scene.h"


void yaget::render::primitives::Mark(const math3d::Vector3& location, const colors::Color& color, float magnitude, Lines& lines)
{
    const math3d::Vector3 xStretch{ magnitude, 0.0f, 0.0f };
    const math3d::Vector3 yStretch{ 0.0f, magnitude, 0.0f };
    const math3d::Vector3 zStretch{ 0.0f, 0.0f, magnitude };

    lines.emplace_back(Line{ location - xStretch, location + xStretch, color, color });
    lines.emplace_back(Line{ location - yStretch, location + yStretch, color, color });
    lines.emplace_back(Line{ location - zStretch, location + zStretch, color, color });
}

void yaget::render::primitives::Outline(float width, float height, const colors::Color& color, Lines& lines)
{
    const math3d::Vector3 topLeft{ 0, 0, 0 };
    const math3d::Vector3 bottomRight{ width, height, 0.0f };

    math3d::Vector3 corner = topLeft;
    math3d::Vector3 nextCorner = bottomRight * math3d::Vector3(1, 0, 0);
    lines.emplace_back(Line{ corner, nextCorner, color, color });

    corner = nextCorner;
    nextCorner = bottomRight;
    lines.emplace_back(Line{ corner, nextCorner, color, color });

    corner = nextCorner;
    nextCorner = bottomRight * math3d::Vector3(0, 1, 0);
    lines.emplace_back(Line{ corner, nextCorner, color, color });

    corner = nextCorner;
    nextCorner = topLeft;
    lines.emplace_back(Line{ corner, nextCorner, color, color });
}

void yaget::render::primitives::Rectangle(const math3d::Vector3& upperLeft, const math3d::Vector3& lowerRight, const colors::Color& color, Lines& lines)
{
    const float z = upperLeft.z;
    math3d::Vector3 corner = upperLeft;
    math3d::Vector3 nextCorner = math3d::Vector3(lowerRight.x, upperLeft.y, z);
    lines.emplace_back(Line{ corner, nextCorner, color, color });

    corner = nextCorner;
    nextCorner = lowerRight;
    lines.emplace_back(Line{ corner, nextCorner, color, color });

    corner = nextCorner;
    nextCorner = math3d::Vector3(upperLeft.x, lowerRight.y, z);
    lines.emplace_back(Line{ corner, nextCorner, color, color });

    corner = nextCorner;
    nextCorner = upperLeft;
    lines.emplace_back(Line{ corner, nextCorner, color, color });
}
