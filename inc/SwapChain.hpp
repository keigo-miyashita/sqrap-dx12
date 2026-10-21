#pragma once

#include "pch.hpp"

#include "Alias.hpp"

namespace sqrp
{
	class Command;
	class Device;
	class Resource;
	class Texture;

	class SwapChain
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const Device* pDevice_ = nullptr;

		ComPtr<IDXGISwapChain4> swapChain_ = nullptr;
		std::vector<TextureHandle> backBuffers_;
		ComPtr<ID3D12DescriptorHeap> rtvHeap_ = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_ = {};
		ComPtr<ID3D12Resource> depthStencilBuffer_ = nullptr;
		ComPtr<ID3D12DescriptorHeap> dsvHeap_ = nullptr;
		D3D12_VIEWPORT viewport_ = {};
		D3D12_RECT scissorsRect_ = {};
		std::wstring name_;
		UINT bufferCount_ = 2;
		void CreateSwapChain(const HWND& hwnd, SIZE winSize);
		void CreateDepthStencilBuffer(SIZE winSize);

	public:
		SwapChain(const Device& device, std::wstring name, const HWND& hwnd, SIZE winSize, UINT bufferCount = 2);
		SwapChain(const SwapChain&) = delete;
		SwapChain& operator=(const SwapChain&) = delete;

		const ComPtr<IDXGISwapChain4>& GetSwapChain() const;
		TextureHandle GetCurrentBackBuffer() const;
		const ComPtr<ID3D12DescriptorHeap>& GetRtvHeap() const;
		D3D12_VIEWPORT GetViewPort() const;
		D3D12_RECT GetRect() const;
		const ComPtr<ID3D12Resource>& GetDepthStencilBuffer() const;
		const ComPtr<ID3D12DescriptorHeap>& GetDsvHeap() const;

	};
}