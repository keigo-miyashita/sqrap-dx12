#include <pch.hpp>
#include <sqrap.hpp>

#include "SampleApplication.hpp"

#ifdef _DEBUG
int main()
{
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif
	auto app = SampleApplication("Rasterize Sample");

	try {
		if (!app.Init()) {
			return -1;
		}
		app.Run();
	}
	catch (const std::exception& e) {
		std::cerr << "例外が発生しました:\n" << e.what() << std::endl;
	}

	return 0;
}