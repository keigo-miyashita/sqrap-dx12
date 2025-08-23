#include "SampleApplication.hpp"

using namespace sqrp;

SampleApplication::SampleApplication(std::string windowName, unsigned int window_width, unsigned int window_height) : Application(windowName, window_width, window_height)
{

}

bool SampleApplication::InitMember()
{
	if (!sampleScene_.Init(*this)) {
		return false;
	}

	return true;
};

void SampleApplication::Render()
{
	sampleScene_.Render();
}

void SampleApplication::Terminate()
{
	UnregisterClass(windowClass_.lpszClassName, windowClass_.hInstance);
};