#include <sqrap.hpp>

#include "SampleApplication.hpp"

#ifdef _DEBUG
int main()
{
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif
	auto app = SampleApplication("Raytracing Sample");

	if (!app.Init()) {
		return -1;
	}
	app.Run();
	app.Terminate();

	return 0;
}