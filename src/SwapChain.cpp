#include "SwapChain.hpp"

#include "Command.hpp"
#include "Device.hpp"
#include "Fence.hpp"
#include "Resource.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	void SwapChain::CreateSwapChain(const HWND& hwnd, SIZE winSize)
	{
		RECT rc = {};
		GetWindowRect(hwnd, &rc);

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc1 = {};
		swapChainDesc1.Width = winSize.cx;
		swapChainDesc1.Height = winSize.cy;
		swapChainDesc1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc1.Stereo = false;
		swapChainDesc1.SampleDesc.Count = 1;
		swapChainDesc1.SampleDesc.Quality = 0;
		swapChainDesc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc1.BufferCount = bufferCount_;
		swapChainDesc1.Scaling = DXGI_SCALING_STRETCH;
		swapChainDesc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc1.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		swapChainDesc1.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		ComPtr<IDXGISwapChain1> swapChain1 = nullptr;
		HRESULT result = device_.GetDXGIFactory()->CreateSwapChainForHwnd(device_.GetCommandQueue(QueueType::Graphics).Get(),
			hwnd,
			&swapChainDesc1,
			nullptr,
			nullptr,
			swapChain1.ReleaseAndGetAddressOf());
		if (FAILED(result)) {
			throw runtime_error("[SwapChain::CreateSwapChain] CreateSwapChainForHwnd failed\nHRESULT = " + to_string(result));
		}
		result = swapChain1.As(&swapChain_);
		if (FAILED(result)) {
			throw runtime_error("[SwapChain::CreateSwapChain] QueryInterface for IDXGISwapChain4 failed\nHRESULT = " + to_string(result));
		}

		result = swapChain_->GetDesc1(&swapChainDesc1);
		if (FAILED(result)) {
			throw runtime_error("[SwapChain::CreateSwapChain] GetDesc1 failed\nHRESULT = " + to_string(result));
		}

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NodeMask = 0;
		heapDesc.NumDescriptors = swapChainDesc1.BufferCount * 2;   // sRGB と UNORM の2つずつ
		result = device_.GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(rtvHeap_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[SwapChain::CreateSwapChain] CreateDescriptorHeap for RTV failed\nHRESULT = " + to_string(result));
		}
		rtvHeap_->SetName((name_ + L"RenderTargetViewHeap").c_str());

		CreateBackBuffers();
	}

	void SwapChain::CreateBackBuffers()
	{
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc1 = {};
		HRESULT result = swapChain_->GetDesc1(&swapChainDesc1);
		if (FAILED(result)) {
			throw runtime_error("[SwapChain::CreateBackBuffers] GetDesc1 failed\nHRESULT = " + to_string(result));
		}

		// Clear old entries left by a previous call (from Resize)
		backBuffers_.clear();
		rtvHandles_.clear();
		unormRtvHandles_.clear();
		backBuffers_.resize(swapChainDesc1.BufferCount);

		D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();

		rtvHandles_.reserve(swapChainDesc1.BufferCount);
		unormRtvHandles_.reserve(swapChainDesc1.BufferCount);
		const UINT rtvIncrement = device_.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		for (UINT i = 0; i < swapChainDesc1.BufferCount; i++) {
			ComPtr<ID3D12Resource> resource;
			result = swapChain_->GetBuffer(i, IID_PPV_ARGS(resource.ReleaseAndGetAddressOf()));
			//backBuffers_[i]->SetResourceState(D3D12_RESOURCE_STATE_RENDER_TARGET);
			if (FAILED(result)) {
				throw runtime_error("[SwapChain::CreateBackBuffers] GetBuffer failed\nHRESULT = " + to_string(result));
			}
			// バックバッファは PRESENT の状態で渡される
			backBuffers_[i] = make_shared<Texture>(device_, name_ + L"_BackBuffer" + to_wstring(i), resource, D3D12_RESOURCE_STATE_PRESENT);
			//rtvDesc.Format = backBuffers_[i]->GetResource()->GetDesc().Format;
			auto rtvDesc = backBuffers_[i]->MakeRTV(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
			device_.GetDevice()->CreateRenderTargetView(backBuffers_[i]->GetResource().Get(), &rtvDesc, handle);
			rtvHandles_.push_back(handle);
			handle.ptr += rtvIncrement;
		}

		// ImGui 用の UNORM の RTV。sRGB の分の後ろに並べる（先頭 N 個の並びは変えない）
		for (UINT i = 0; i < swapChainDesc1.BufferCount; i++) {
			auto unormRtvDesc = backBuffers_[i]->MakeRTV(DXGI_FORMAT_R8G8B8A8_UNORM);
			device_.GetDevice()->CreateRenderTargetView(backBuffers_[i]->GetResource().Get(), &unormRtvDesc, handle);
			unormRtvHandles_.push_back(handle);
			handle.ptr += rtvIncrement;
		}

		viewport_ = CD3DX12_VIEWPORT(backBuffers_[0]->GetResource().Get());
		scissorsRect_ = CD3DX12_RECT(0, 0, swapChainDesc1.Width, swapChainDesc1.Height);
	}

	void SwapChain::CreateDepthStencilBuffer(SIZE winSize)
	{
		auto depthResDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT,
			winSize.cx, winSize.cy, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

		auto depthHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		CD3DX12_CLEAR_VALUE depthClearValue(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);

		ComPtr<ID3D12Resource> depthResource;
		HRESULT result = device_.GetDevice()->CreateCommittedResource(
			&depthHeapProp, D3D12_HEAP_FLAG_NONE,
			&depthResDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthClearValue, IID_PPV_ARGS(depthResource.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[SwapChain::CreateDepthStencilBuffer] CreateCommittedResource failed\nHRESULT = " + to_string(result));
		}
		depthStencilBuffer_ = make_shared<Texture>(device_, name_ + L"DepthStencilBuffer", depthResource, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		// The DSV heap does not depend on the size, so create it once and reuse it on Resize
		if (!dsvHeap_) {
			D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
			dsvHeapDesc.NumDescriptors = 1;
			dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

			result = device_.GetDevice()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(dsvHeap_.ReleaseAndGetAddressOf()));
			if (FAILED(result)) {
				throw runtime_error("[SwapChain::CreateDepthStencilBuffer] CreateDescriptorHeap for DSV failed\nHRESULT = " + to_string(result));
			}
			dsvHeap_->SetName((name_ + L"DepthStencilHeap").c_str());
		}

		auto dsvDesc = depthStencilBuffer_->MakeDSV();
		device_.GetDevice()->CreateDepthStencilView(depthStencilBuffer_->GetResource().Get(), &dsvDesc, dsvHeap_->GetCPUDescriptorHandleForHeapStart());
	}

	SwapChain::SwapChain(const Device& device, wstring name, const HWND& hwnd, SIZE winSize, UINT bufferCount)
		: device_(device), name_(name), bufferCount_(bufferCount)
	{
		if (bufferCount_ < 2 || bufferCount_ > DXGI_MAX_SWAP_CHAIN_BUFFERS) {
			throw runtime_error("[SwapChain::SwapChain] bufferCount must be between 2 and " + to_string(DXGI_MAX_SWAP_CHAIN_BUFFERS));
		}

		CreateSwapChain(hwnd, winSize);

		CreateDepthStencilBuffer(winSize);
	}

	void SwapChain::Resize(UINT width, UINT height)
	{
		// Minimizing gives 0. Keep the current buffers until the window is restored
		if (width == 0 || height == 0) {
			return;
		}

		// Wait until the GPU is done with the buffers, including Present.
		// The Signal in EndRender comes before Present, so signal again here and wait for it
		FenceHandle fence = device_.GetFence(QueueType::Graphics);
		device_.WaitOnCPU(TimelinePoint(QueueType::Graphics, fence->Signal()));

		// ResizeBuffers fails while any reference to a back buffer remains
		backBuffers_.clear();
		depthStencilBuffer_.reset();

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc1 = {};
		HRESULT result = swapChain_->GetDesc1(&swapChainDesc1);
		if (FAILED(result)) {
			throw runtime_error("[SwapChain::Resize] GetDesc1 failed\nHRESULT = " + to_string(result));
		}
		result = swapChain_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, swapChainDesc1.Flags);
		if (FAILED(result)) {
			throw runtime_error("[SwapChain::Resize] ResizeBuffers failed (a TextureHandle from GetCurrentBackBuffer() may still be held)\nHRESULT = " + to_string(result));
		}

		CreateBackBuffers();
		CreateDepthStencilBuffer({ static_cast<LONG>(width), static_cast<LONG>(height) });
	}

	const ComPtr<IDXGISwapChain4>& SwapChain::GetSwapChain() const
	{
		return swapChain_;
	}

	TextureHandle SwapChain::GetCurrentBackBuffer() const
	{
		auto bbIndex = swapChain_->GetCurrentBackBufferIndex();
		return backBuffers_[bbIndex];
	}

	const ComPtr<ID3D12DescriptorHeap>& SwapChain::GetRtvHeap() const
	{
		return rtvHeap_;
	}

	D3D12_VIEWPORT SwapChain::GetViewPort() const
	{
		return viewport_;
	}

	D3D12_RECT SwapChain::GetRect() const
	{
		return scissorsRect_;
	}

	const TextureHandle& SwapChain::GetDepthStencilBuffer() const
	{
		return depthStencilBuffer_;
	}

	const ComPtr<ID3D12DescriptorHeap>& SwapChain::GetDsvHeap() const
	{
		return dsvHeap_;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE SwapChain::GetRtvHandle(UINT bufferIndex) const
	{
		return rtvHandles_[bufferIndex];
	}

	D3D12_CPU_DESCRIPTOR_HANDLE SwapChain::GetUnormRtvHandle(UINT bufferIndex) const
	{
		return unormRtvHandles_[bufferIndex];
	}
}