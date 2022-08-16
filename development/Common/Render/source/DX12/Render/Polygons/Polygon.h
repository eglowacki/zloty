/////////////////////////////////////////////////////////////////////////
// Polygon.h
//
//  Copyright 08/15/2022 Edgar Glowacki.
//
// NOTES:
//      First attempt at creating a renderable object in DX 12, like a triangle or a model composed of triangles
//
// #include "Render/Polygons/Polygon.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Render/RenderCore.h"

struct ID3D12Resource;
namespace D3D12MA
{
	class Allocation;
	class Allocator;
}

namespace yaget::render
{
	class Polygon
	{
	public:
		Polygon(D3D12MA::Allocator* allocator);
		~Polygon();

	private:
		ComPtr<ID3D12Resource> mTriangleData;
		D3D12MA::Allocation* mAllocation = nullptr;
	};
}
