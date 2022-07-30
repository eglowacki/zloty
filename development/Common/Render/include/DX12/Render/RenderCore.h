/////////////////////////////////////////////////////////////////////////
// RenderCore.h
//
//  Copyright TODAYS_DATE_GOES_HERE Edgar Glowacki.
//
// NOTES:
//      
//
// #include "Render/RenderCore.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "YagetCore.h"
#include <wrl/client.h>

namespace yaget::render
{
	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
}
