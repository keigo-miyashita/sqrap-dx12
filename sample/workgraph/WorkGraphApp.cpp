#include "WorkGraphApp.hpp"

using namespace sqrp;

WorkGraphApp::WorkGraphApp(std::string windowName, unsigned int window_width, unsigned int window_height) : Application(windowName, window_width, window_height)
{

}

bool WorkGraphApp::InitMember()
{
	if (!sampleScene_.Init(*this))
	{
		return false;
	}
	return true;
};

void WorkGraphApp::Render()
{
	sampleScene_.Render();
};

void WorkGraphApp::Terminate()
{
	UnregisterClass(windowClass_.lpszClassName, windowClass_.hInstance);
};