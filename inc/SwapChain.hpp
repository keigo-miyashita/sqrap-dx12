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

		const Device& device_;

		ComPtr<IDXGISwapChain4> swapChain_ = nullptr;
		std::vector<TextureHandle> backBuffers_;
		ComPtr<ID3D12DescriptorHeap> rtvHeap_ = nullptr;
		// RTV はバックバッファ1枚につき2つ。ヒープの先頭 N 個が sRGB（場面の描画用。書くときに
		// 線形 -> sRGB に符号化する）、後ろの N 個が UNORM（ImGui 用。ImGui の色は画面に出す値
		// そのものなので、符号化すると白っぽくなる）
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles_ = {};
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> unormRtvHandles_ = {};
		TextureHandle depthStencilBuffer_ = nullptr;
		ComPtr<ID3D12DescriptorHeap> dsvHeap_ = nullptr;
		D3D12_VIEWPORT viewport_ = {};
		D3D12_RECT scissorsRect_ = {};
		std::wstring name_;
		UINT bufferCount_ = 2;
		void CreateSwapChain(const HWND& hwnd, SIZE winSize);
		void CreateBackBuffers();
		void CreateDepthStencilBuffer(SIZE winSize);

	public:
		SwapChain(const Device& device, std::wstring name, const HWND& hwnd, SIZE winSize, UINT bufferCount = 2);
		SwapChain(const SwapChain&) = delete;
		SwapChain& operator=(const SwapChain&) = delete;
		void Resize(UINT width, UINT height);

		const ComPtr<IDXGISwapChain4>& GetSwapChain() const;
		TextureHandle GetCurrentBackBuffer() const;
		const ComPtr<ID3D12DescriptorHeap>& GetRtvHeap() const;
		D3D12_VIEWPORT GetViewPort() const;
		D3D12_RECT GetRect() const;
		const TextureHandle& GetDepthStencilBuffer() const;
		const ComPtr<ID3D12DescriptorHeap>& GetDsvHeap() const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle(UINT bufferIndex) const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetUnormRtvHandle(UINT bufferIndex) const;
	};
}