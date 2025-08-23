#include "MeshShaderApp.hpp"

using namespace sqrp;

MeshShaderApp::MeshShaderApp(std::string windowName, unsigned int window_width, unsigned int window_height) : Application(windowName, window_width, window_height)
{

}

bool MeshShaderApp::InitMember()
{
	if (!sampleScene_.Init(*this)) {
		return false;
	}

	return true;
};

void MeshShaderApp::Render()
{
	sampleScene_.Render();
};

void MeshShaderApp::Terminate()
{
	UnregisterClass(windowClass_.lpszClassName, windowClass_.hInstance);
};