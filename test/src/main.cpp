#include <common.hpp>

#include "TestApplication.hpp"

#ifdef _DEBUG
int main()
{
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif
	auto app = TestApplication("Test");

	Microsoft::WRL::ComPtr<ID3D12DebugDevice> debugDevice_ = nullptr;

	if (!app.Init(debugDevice_)) {
		return -1;
	}
	app.Run();
	app.Terminate();

	/*auto result = debugDevice_->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
	if (FAILED(result)) {
		std::cerr << "Failed to ReportLiveDeviceObjects" << std::endl;
		return -1;
	}*/

	return 0;
}