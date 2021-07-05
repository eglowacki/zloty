/////////////////////////////////////////////////////////////////////////
// WindowFrame.h
//
//  Copyright 06/20/2021 Edgar Glowacki.
//
// NOTES:
//      Wrapper class for getting display surface, which provides
//      window handle and size in application agnostic way.
//
// #include "App/WindowFrame.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "YagetCore.h"
#include "App/Display.h"


namespace yaget { class Application; }

namespace yaget::app
{
	class WindowFrame
	{
	public:
		WindowFrame(const Application& app);
		~WindowFrame() = default;

		app::DisplaySurface GetSurface() const;
		
	private:
		const Application& mApplication;
	};
}
