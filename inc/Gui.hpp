#pragma once

#include <common.hpp>

class Device;

class GUI
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	// D3D12 descriptor heap
	ComPtr<ID3D12DescriptorHeap> imguiDescHeap_ = nullptr;

	bool InitializeGui(const Device& device, const HWND& hwnd);


public:
	GUI();
	~GUI() = default;
	bool Init();
	ComPtr<ID3D12DescriptorHeap> GetImguiDescHeap();
};