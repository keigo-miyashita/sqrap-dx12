#pragma once

#include <common.hpp>

class CommandManager;
class Device;

class GUI
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	Device* pDevice_ = nullptr;
	// D3D12 descriptor heap
	ComPtr<ID3D12DescriptorHeap> imguiDescHeap_ = nullptr;

	bool InitializeGUI(const HWND& hwnd);


public:
	GUI();
	~GUI();
	bool Init(Device* pDevice, const HWND& hwnd);
	void BeginCommand();
	void EndCommand();
	void Draw(CommandManager& commandManager);
	ComPtr<ID3D12DescriptorHeap> GetImguiDescHeap() const;
};