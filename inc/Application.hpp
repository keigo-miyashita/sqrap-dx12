#pragma once

#include <common.hpp>

class Application
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

protected:
	std::string windowName_ = "Application";
	HINSTANCE hInstance_ = 0;
	WNDCLASSEX windowClass_ = { 0 }; // If it isn't initialized as 0, window cannot create !!!
	HWND hwnd_ = 0;
	unsigned int window_width_ = 1280;
	unsigned int window_height_ = 720;

public:
	Application();
	Application(std::string windowName, unsigned int window_width = 1280, unsigned int window_height = 720);
	~Application() = default;
	// WindowÇÃèâä˙âªä÷êî
	void CreateGameWindow(std::string windowName);
	virtual int Input(UINT msg, WPARAM wparam, LPARAM lparam);
	virtual bool Init();
	virtual void Run();
	virtual void Terminate();
	HWND GetWindowHWND() const;
	UINT GetWindowWidth() const ;
	UINT GetWindowHeight() const;
};