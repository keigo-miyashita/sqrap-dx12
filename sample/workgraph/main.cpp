#include <pch.hpp>
#include <sqrap.hpp>

#include "WorkGraphApp.hpp"

#ifdef _DEBUG
int main()
{
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif
	auto app = WorkGraphApp("WorkGraph Sample");

	//try {
		if (!app.Init()) {
			return -1;
		}
		app.Run();
		app.Terminate();
	/*}
	catch (const std::exception& e) {
		std::cerr << "—áŠO‚ª”­¶‚µ‚Ü‚µ‚½: " << e.what() << std::endl;
	}*/

	return 0;
}