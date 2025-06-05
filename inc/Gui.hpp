#pragma once

#include <common.hpp>

class Command;
class Device;

class GUI
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const Device* pDevice_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> imguiDescHeap_ = nullptr;

	bool InitializeGUI(const HWND& hwnd);


public:
	GUI(const Device& device, const HWND& hwnd);
	~GUI();
	//bool Init(Device* pDevice, const HWND& hwnd);
	void BeginCommand();
	void EndCommand();
	void Draw(Command& command);
	ComPtr<ID3D12DescriptorHeap> GetImguiDescHeap() const;
};