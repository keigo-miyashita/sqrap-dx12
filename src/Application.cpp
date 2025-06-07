#include "Application.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

void Input::GetRawState(UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (ImGui::GetCurrentContext() != nullptr) {
		ImGuiIO& io = ImGui::GetIO();
		if (io.WantCaptureMouse) {
			return;
		}
	}

	UINT vkCode = (UINT)wparam;
	int xPos = GET_X_LPARAM(lparam);
	int yPos = GET_Y_LPARAM(lparam);
	currentMousePos_ = {xPos, yPos};
	/*for (auto& eachIsPushKey : isPushKey_) {
		eachIsPushKey.second = false;
	}*/
	switch (msg) {
	case WM_KEYDOWN:
		//if ((lparam & (1 << 30)) == 0) {
			isPushKey_[vkCode] = true;
		//}
		cout << "isPushKey" << endl;
		break;
	case WM_KEYUP:
		isPushKey_[vkCode] = false;
		cout << "isUnPushKey" << endl;
		break;
	case WM_LBUTTONDOWN:
		isPushedLButton_ = true;
		pushedMousePos_ = { xPos, yPos };
		break;
	case WM_LBUTTONUP:
		isPushedLButton_ = false;
		break;
	case WM_MOUSEWHEEL:
		short wheelDelta = GET_WHEEL_DELTA_WPARAM(wparam);
		wheel_ = wheelDelta;
		break;
	}
}

void Input::Update()
{
	for (auto& [vk, rawState] : isPushKey_) {
		bool prev = isLogicalPushKey_[vk].isRawPushed;
		isLogicalPushKey_[vk].isRawPushed = rawState;

		if (!prev && rawState) {
			isLogicalPushKey_[vk].isPushed = true;
		}
		if (prev && !rawState) {
			isLogicalPushKey_[vk].isPushed = false;
		}
		//isLogicalPushKey_[vk].isPushed = !prev && rawState;    // 前がfalse、今がtrue → 押された
		//isLogicalPushKey_[vk].isReleased = prev && !rawState;   // 前がtrue、今がfalse → 離された
	}
}

bool Input::IsPushKey(UINT key)
{
	return isLogicalPushKey_[key].isPushed;
}

int Input::GetWheel()
{
	return wheel_;
}

MousePosition Input::GetPushedPos()
{
	return pushedMousePos_;
}

MousePosition Input::GetPos()
{
	return currentMousePos_;
}

bool Input::IsPushedLButton()
{
	return isPushedLButton_;
}

bool Input::IsPushedRButton()
{
	return isPushedRButton_;
}

namespace {

	Application* appPtr = nullptr;

	LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		int x, y, wheelDelta;
		int delta = GET_WHEEL_DELTA_WPARAM(wparam);
		Application* app = reinterpret_cast<Application*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);

		switch (msg) {
			case WM_DESTROY:
				PostQuitMessage(0);
				return 0;
		}

		if (appPtr) {
			appPtr->Input(msg, wparam, lparam);
		}
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
}

void Application::CreateGameWindow(std::string windowName)
{
	hInstance_ = GetModuleHandle(nullptr);
	// Create Window class
	windowClass_.cbSize = sizeof(WNDCLASSEX);
	windowClass_.lpfnWndProc = (WNDPROC)WindowProcedure;
	windowClass_.lpszClassName = _T(windowName.c_str());
	windowClass_.hInstance = GetModuleHandle(0);

	if (!RegisterClassEx(&windowClass_)) {
		cerr << "Error on RegisterClassEx : " << GetLastError() << endl;
	}

	// Window size
	RECT wrc = { 0, 0, window_width_, window_height_ };
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
	// Create window object
	hwnd_ = CreateWindow(
		windowClass_.lpszClassName,
		_T(windowName.c_str()),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wrc.right - wrc.left,
		wrc.bottom - wrc.top,
		nullptr,
		nullptr,
		windowClass_.hInstance,
		nullptr);

	if (hwnd_ == nullptr) {
		cerr << "Cannot create HWND" << endl;
	}

	SetWindowLongPtr(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
}

Application::Application()
{
	appPtr = this;
}

Application::Application(std::string windowName, unsigned int window_width, unsigned int window_height)
{
	appPtr = this;
	windowName_ = windowName;
	window_width_ = window_width;
	window_height_ = window_height;
}

int Application::Input(UINT msg, WPARAM wparam, LPARAM lparam)
{
	Input::GetRawState(msg, wparam, lparam);

	return 0;
}

bool Application::Init()
{
	return 0;
}

void Application::Run()
{

}

void Application::Terminate()
{
	
}

HWND Application::GetWindowHWND() const
{
	return hwnd_;
}

UINT Application::GetWindowWidth() const
{
	return window_width_;
}

UINT Application::GetWindowHeight() const
{
	return window_height_;
}