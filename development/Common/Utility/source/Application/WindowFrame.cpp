#include "App/WindowFrame.h"
#include "App/Application.h"


//-------------------------------------------------------------------------------------------------
yaget::app::WindowFrame::WindowFrame(const Application& app)
	: mApplication(app)
{
}


//-------------------------------------------------------------------------------------------------
yaget::app::DisplaySurface yaget::app::WindowFrame::GetSurface() const
{
	return mApplication.GetSurface();
}
