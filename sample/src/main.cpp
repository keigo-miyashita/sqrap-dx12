#include <common.hpp>

#include "SampleApplication.hpp"

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION_MACRO; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = D3D12_SDK_PATH_MACRO; }

#ifdef _DEBUG
int main()
{
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif
	auto app = SampleApplication("Sample");

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