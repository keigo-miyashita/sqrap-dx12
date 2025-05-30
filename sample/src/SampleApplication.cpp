#include <common.hpp>

#include "SampleApplication.hpp"

SampleApplication::SampleApplication(std::string windowName, unsigned int window_width, unsigned int window_height) : Application(windowName, window_width, window_height)
{

}

bool SampleApplication::Init()
{
	// Comポインタを使う準備
	// 第二引数はマルチスレッドへの対応
	if (FAILED(CoInitializeEx(0, COINIT_MULTITHREADED))) {
		return false;
	}
	CreateGameWindow(windowName_);

	sampleScene_.Init(*this);
};

bool SampleApplication::Init(ComPtr<ID3D12DebugDevice>& debugDevice)
{
	if (FAILED(CoInitializeEx(0, COINIT_MULTITHREADED))) {
		std::cerr << "Failed to CoInitializeEx" << std::endl;
		return false;
	}
	CreateGameWindow(windowName_);

	if (!sampleScene_.Init(*this, debugDevice)) {
		return false;
	}

	return true;
};

void SampleApplication::Run()
{
	ShowWindow(hwnd_, SW_SHOW);
	MSG msg = {};

	while (true) {

		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT) {
			break;
		}

		sampleScene_.Render();
	}
};

void SampleApplication::Terminate()
{
	UnregisterClass(windowClass_.lpszClassName, windowClass_.hInstance);
};