#pragma once

#include <common.hpp>

class Command;
class Device;

class SwapChain
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const Device* pDevice_ = nullptr;
	std::shared_ptr<Command> pCommand_;

	ComPtr<IDXGISwapChain4> swapChain_ = nullptr;
	std::vector<ComPtr<ID3D12Resource>> backBuffers_;
	ComPtr<ID3D12DescriptorHeap> rtvHeap_ = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_;
	ComPtr<ID3D12Resource> depthStencilBuffer_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> dsvHeap_ = nullptr;
	D3D12_VIEWPORT viewport_;
	D3D12_RECT scissorsRect_;
	std::wstring name_;
	bool CreateSwapChain(const HWND& hwnd, SIZE winSize);
	bool CreateDepthStencilBuffer(SIZE winSize);


public:
	SwapChain(const Device& device, std::shared_ptr<Command>& command, const HWND& hwnd, SIZE winSize, std::wstring name = L"");
	~SwapChain() = default;
	ComPtr<IDXGISwapChain4> GetSwapChain();
	ComPtr<ID3D12Resource> GetCurrentBackBuffer(UINT index);
	ComPtr<ID3D12DescriptorHeap> GetRtvHeap();
	D3D12_VIEWPORT GetViewPort();
	D3D12_RECT GetRect();
	ComPtr<ID3D12Resource> GetDepthStencilBuffer();
	ComPtr<ID3D12DescriptorHeap> GetDsvHeap();

};