#include "Swapchain.hpp"

#include "Command.hpp"
#include "Device.hpp"
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
		swapChainDesc1.BufferCount = 2;
		swapChainDesc1.Scaling = DXGI_SCALING_STRETCH;
		swapChainDesc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc1.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		swapChainDesc1.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		HRESULT result = pDevice_->GetDXGIFactory()->CreateSwapChainForHwnd(pDevice_->GetGraphicsCommandQueue().Get(),
			hwnd,
			&swapChainDesc1,
			nullptr,
			nullptr,
			(IDXGISwapChain1**)swapChain_.ReleaseAndGetAddressOf());
		if (FAILED(result)) {
			throw std::runtime_error("Failed to CreateSwapChainForHwnd : " + to_string(result));
		}

		result = swapChain_->GetDesc1(&swapChainDesc1);
		if (FAILED(result)) {
			throw std::runtime_error("Failed to get swapChainDesc1 : " + to_string(result));
		}

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NodeMask = 0;
		heapDesc.NumDescriptors = 2;
		result = pDevice_->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(rtvHeap_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw std::runtime_error("Failed to CreateDescriptorHeap for SwapChain : " + to_string(result));
		}
		rtvHeap_->SetName((name_ + L"RenderTargetViewHeap").c_str());

		backBuffers_.resize(swapChainDesc1.BufferCount);

		D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		for (int i = 0; i < swapChainDesc1.BufferCount; i++) {
			backBuffers_[i] = make_shared<Texture>();
			ComPtr<ID3D12Resource> resource;
			result = swapChain_->GetBuffer(i, IID_PPV_ARGS(resource.ReleaseAndGetAddressOf()));
			backBuffers_[i]->SetResource(resource);
			backBuffers_[i]->SetResourceState(D3D12_RESOURCE_STATE_RENDER_TARGET);
			if (FAILED(result)) {
				throw std::runtime_error("Failed to GetBuffer for SwapChain : " + to_string(result));
			}
			wstring nameBackBuffer = L"backBuffers" + to_wstring(i);
			backBuffers_[i]->SetName(nameBackBuffer.c_str());
			rtvDesc.Format = backBuffers_[i]->GetResource()->GetDesc().Format;
			pDevice_->GetDevice()->CreateRenderTargetView(backBuffers_[i]->GetResource().Get(), &rtvDesc, handle);
			handle.ptr += pDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
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

		HRESULT result = pDevice_->GetDevice()->CreateCommittedResource(
			&depthHeapProp, D3D12_HEAP_FLAG_NONE,
			&depthResDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthClearValue, IID_PPV_ARGS(depthStencilBuffer_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw std::runtime_error("Failed to CreateCommittedResource for SwapChain's DepthStencilBuffer : " + to_string(result));
		}
		depthStencilBuffer_->SetName((name_ + L"DepthStencilBuffer").c_str());

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		result = pDevice_->GetDevice()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(dsvHeap_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw std::runtime_error("Failed to CreateDescriptorHeap for SwapChain's DepthStencilBuffer : " + to_string(result));
		}
		dsvHeap_->SetName((name_ + L"DepthStencilHeap").c_str());

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		pDevice_->GetDevice()->CreateDepthStencilView(depthStencilBuffer_.Get(), &dsvDesc, dsvHeap_->GetCPUDescriptorHandleForHeapStart());
	}

	SwapChain::SwapChain(const Device& device, std::wstring name, shared_ptr<Command> command, const HWND& hwnd, SIZE winSize)
		: pDevice_(&device), pCommand_(command), name_(name)
	{

		CreateSwapChain(hwnd, winSize);

		CreateDepthStencilBuffer(winSize);
	}

	void SwapChain::BeginRender()
	{
		auto bbIdx = swapChain_->GetCurrentBackBufferIndex();

		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer()->GetResource().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		pCommand_->GetCommandList()->ResourceBarrier(1, &barrier);

		auto rtvHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += bbIdx * pDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		auto dsvHandle = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
		pCommand_->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

		// Clear depth buffer
		pCommand_->GetCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		// Clear display
		float clearColor[] = { 0.5f, 0.5f, 0.5f, 1.0f };
		pCommand_->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		// Set viewport and scissors
		pCommand_->GetCommandList()->RSSetViewports(1, &viewport_);
		pCommand_->GetCommandList()->RSSetScissorRects(1, &scissorsRect_);
	}

	void SwapChain::EndRender()
	{
		auto bbIdx = swapChain_->GetCurrentBackBufferIndex();

		// Transit render target to present
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer()->GetResource().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		pCommand_->GetCommandList()->ResourceBarrier(1, &barrier);

		pCommand_->WaitCommand();

		swapChain_->Present(1, 0);
	}

	ComPtr<IDXGISwapChain4> SwapChain::GetSwapChain()
	{
		return swapChain_;
	}

	TextureHandle SwapChain::GetCurrentBackBuffer()
	{
		auto bbIndex = swapChain_->GetCurrentBackBufferIndex();
		return backBuffers_[bbIndex];
	}

	ComPtr<ID3D12DescriptorHeap> SwapChain::GetRtvHeap()
	{
		return rtvHeap_;
	}

	D3D12_VIEWPORT SwapChain::GetViewPort()
	{
		return viewport_;
	}

	D3D12_RECT SwapChain::GetRect()
	{
		return scissorsRect_;
	}

	ComPtr<ID3D12Resource> SwapChain::GetDepthStencilBuffer()
	{
		return depthStencilBuffer_;
	}

	ComPtr<ID3D12DescriptorHeap> SwapChain::GetDsvHeap()
	{
		return dsvHeap_;
	}
}