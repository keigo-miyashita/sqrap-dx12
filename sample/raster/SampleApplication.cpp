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

void SampleApplication::Run()
{
	ShowWindow(hwnd_, SW_SHOW);
	MSG msg = {};

	bool isRunning = true;

	while (isRunning) {

		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				isRunning = false;
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		Input::Update();

		sampleScene_.Render();
	}
};

void SampleApplication::Terminate()
{
	UnregisterClass(windowClass_.lpszClassName, windowClass_.hInstance);
};