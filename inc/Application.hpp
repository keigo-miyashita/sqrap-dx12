#pragma once

#include "pch.hpp"

namespace sqrp
{
	struct MousePosition
	{
		int x;
		int y;
	};

	class Input
	{
	private:

		struct InputLogicalState
		{
			bool isRawPushed = false;
			bool isPushed = false;
		};

		static inline std::unordered_map<UINT, bool> isPushKey_ = []() {
			std::unordered_map<UINT, bool> m;
			for (UINT k = 'A'; k <= 'Z'; ++k) m[k] = false;
			m[VK_SPACE] = false;
			m[VK_CONTROL] = false;
			return m;
		}();
		static inline std::unordered_map<UINT, InputLogicalState> isLogicalPushKey_ = []() {
			std::unordered_map<UINT, InputLogicalState> m;
			for (UINT k = 'A'; k <= 'Z'; ++k) m[k] = {};
			m[VK_SPACE] = {};
			m[VK_CONTROL] = {};
			return m;
		}();
		static inline int wheel_ = 0;
		static inline bool isPushedLButton_ = false;
		static inline bool isPushedRButton_ = false;
		static inline MousePosition pushedMousePos_;
		static inline MousePosition prevMousePos_;
		static inline MousePosition currentMousePos_;
		static inline MousePosition deltaMousePos_;

	public:
		static void GetRawState(UINT msg, WPARAM wparam, LPARAM lparam);
		static void Update();
		static bool IsPushKey(UINT key);
		static int GetWheel();
		static MousePosition GetPushedPos();
		static MousePosition GetPrevPos();
		static MousePosition GetDeltaPos();
		static MousePosition GetPos();
		static bool IsPushedLButton();
		static bool IsPushedRButton();
	};

	class Application
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		// Window�̏������֐�
		void CreateGameWindow(std::string windowName);

	protected:
		std::string windowName_ = "Application";
		HINSTANCE hInstance_ = 0;
		WNDCLASSEX windowClass_ = { 0 }; // If it isn't initialized as 0, window cannot create !!!
		HWND hwnd_ = 0;
		unsigned int windowWidth_ = 1280;
		unsigned int windowHeight_ = 720;

	public:
		Application();
		Application(std::string windowName, unsigned int windowWidth = 1280, unsigned int windowHeight = 720);
		~Application() = default;
		bool Input(UINT msg, WPARAM wparam, LPARAM lparam);
		bool Init();
		void Run();
		virtual bool OnStart();
		virtual void OnUpdate();
		virtual void OnTerminate();
		HWND GetWindowHWND() const;
		UINT GetWindowWidth() const;
		UINT GetWindowHeight() const;
	};
}