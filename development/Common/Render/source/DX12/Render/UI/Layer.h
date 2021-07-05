/////////////////////////////////////////////////////////////////////////
// Layer.h
//
//  Copyright 06/24/2021 Edgar Glowacki.
//
// NOTES:
//		Some abstraction interface for dealing with ui controls
//		and rendering provided by ImGui.
//      
//
// #include "Render/UI/Layer.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "YagetCore.h"

namespace yaget {
	namespace app {
		class DisplaySurface;
	}
}

namespace yaget::render::ui
{
	class Layer
	{
	public:
		class Resizer
		{
		public:
			Resizer(Layer& layer);
			~Resizer();

		private:
			Layer& mLayer;
		};
		
		Layer(const app::DisplaySurface& displaySurface);
		~Layer();

		void Begin();
		void End();

	private:
	};
}
