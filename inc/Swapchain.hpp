#pragma once

#include <common.hpp>

class CommandManager;
class Device;

class SwapChain
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ComPtr<IDXGISwapChain4> swapChain_ = nullptr;
	std::vector<ComPtr<ID3D12Resource>> backBuffers_;
	ComPtr<ID3D12DescriptorHeap> rtvHeap_ = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_;
	D3D12_VIEWPORT viewport_;
	D3D12_RECT scissorsRect_;
	ComPtr<ID3D12Resource> depthStencilBuffer_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> dsvHeap_ = nullptr;
	bool CreateSwapChain(const Device& device, const HWND& hwnd, SIZE winSize, const CommandManager& commandQueue, std::wstring name = L"SwapChain");
	bool CreateDepthStencilBuffer(const Device& device, SIZE winSize, std::wstring name = L"SwapChain");


public:
	SwapChain();
	~SwapChain() = default;
	bool Init(const Device& device, const HWND& hwnd, SIZE winSize, const CommandManager& commandQueue, std::wstring name = L"SwapChain");
	ComPtr<IDXGISwapChain4> GetSwapChain();
	ComPtr<ID3D12Resource> GetCurrentBackBuffer(UINT index);
	ComPtr<ID3D12DescriptorHeap> GetRtvHeap();
	D3D12_VIEWPORT GetViewPort();
	D3D12_RECT GetRect();
	ComPtr<ID3D12Resource> GetDepthStencilBuffer();
	ComPtr<ID3D12DescriptorHeap> GetDsvHeap();

};