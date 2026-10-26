#include <pch.hpp>
#include <sqrap.hpp>

#include "MeshShaderApp.hpp"

#ifdef _DEBUG
int main()
{
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif
	auto app = MeshShaderApp("WorkGraph Sample");

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