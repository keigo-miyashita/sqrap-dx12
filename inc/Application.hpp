#pragma once

#include <common.hpp>

struct MousePosition
{
	int x;
	int y;
};

class InputManager
{
private:
	static inline std::unordered_map<UINT, bool> isPushKey_ = {
		{'A', false},
		{'B', false},
		{'C', false},
		{'D', false},
		{'E', false},
		{'F', false},
		{'G', false},
		{'H', false},
		{'I', false},
		{'J', false},
		{'K', false},
		{'L', false},
		{'M', false},
		{'N', false},
		{'O', false},
		{'P', false},
		{'Q', false},
		{'R', false},
		{'S', false},
		{'T', false},
		{'U', false},
		{'V', false},
		{'W', false},
		{'X', false},
		{'Y', false},
		{'Z', false},
		{VK_SPACE, false},
		{VK_CONTROL, false},
	};
	static inline int wheel_ = 0;
	static inline bool isPushedLButton_ = false;
	static inline bool isPushedRButton_ = false;
	static inline MousePosition pushedMousePos_;
	static inline MousePosition currentMousePos_;

public:
	static void Update(UINT msg, WPARAM wparam, LPARAM lparam);
	static bool IsPushKey(UINT key);
	static int GetWheel();
	static MousePosition GetPushedPos();
	static MousePosition GetPos();
	static bool IsPushedLButton();
	static bool IsPushedRButton();
};

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