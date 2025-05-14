#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

bool SwapChain::CreateSwapChain(const Device& device, const HWND& hwnd, SIZE winSize, const CommandManager& commandManager, wstring name)
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

	if (FAILED(device.GetDXGIFactory()->CreateSwapChainForHwnd(commandManager.GetCommandQueue().Get(),
													hwnd,
													&swapChainDesc1,
													nullptr,
													nullptr,
													(IDXGISwapChain1**)swapChain_.ReleaseAndGetAddressOf()))) {
		return false;
	}

	if (FAILED(swapChain_->GetDesc1(&swapChainDesc1))) {
		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;
	if (FAILED(device.GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(rtvHeap_.ReleaseAndGetAddressOf())))) {
		return false;
	}
	rtvHeap_->SetName(L"RenderTargetViewHeap");
	
	// DESC1‚¶‚á‚È‚­‚ÄDESC‚ðŽæ“¾‚µ‚Ä‚¢‚é‚ªDESC1‚Ì‚Ü‚Ü‚Å‚æ‚¢‚Ì‚Å‚ÍH
	/*DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	if (FAILED(swapChain_->GetDesc(&swapChainDesc))) {
		return false;
	}
	backBuffers_.resize(swapChainDesc.BufferCount);*/
	backBuffers_.resize(swapChainDesc1.BufferCount);

	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < swapChainDesc1.BufferCount; i++) {
		if (swapChain_->GetBuffer(i, IID_PPV_ARGS(&backBuffers_[i]))) {
			cerr << "Failed to get buffer of swapchain\n";
			return false;
		}
		wstring nameBackBuffer = L"backBuffers" + to_wstring(i);
		backBuffers_[i]->SetName(nameBackBuffer.c_str());
		rtvDesc.Format = backBuffers_[i]->GetDesc().Format;
		device.GetDevice()->CreateRenderTargetView(backBuffers_[i].Get(), &rtvDesc, handle);
		handle.ptr += device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	viewport_ = CD3DX12_VIEWPORT(backBuffers_[0].Get());
	scissorsRect_ = CD3DX12_RECT(0, 0, swapChainDesc1.Width, swapChainDesc1.Height);

	return true;
}

bool SwapChain::CreateDepthStencilBuffer(const Device& device, SIZE winSize, wstring name)
{
	auto depthResDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT,
		winSize.cx, winSize.cy, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	auto depthHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	CD3DX12_CLEAR_VALUE depthClearValue(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);

	if (FAILED(device.GetDevice()->CreateCommittedResource(
		&depthHeapProp, D3D12_HEAP_FLAG_NONE, 
		&depthResDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue, IID_PPV_ARGS(depthStencilBuffer_.ReleaseAndGetAddressOf())))) {
		return false;
	}
	depthStencilBuffer_->SetName(L"DepthStencilBuffer");

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	if (FAILED(device.GetDevice()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(dsvHeap_.ReleaseAndGetAddressOf())))) {
		return false;
	}
	dsvHeap_->SetName(L"DepthStencilHeap");

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	device.GetDevice()->CreateDepthStencilView(depthStencilBuffer_.Get(), &dsvDesc, dsvHeap_->GetCPUDescriptorHandleForHeapStart());

	return true;
}

SwapChain::SwapChain()
{

}

bool SwapChain::Init(const Device& device, const HWND& hwnd, SIZE winSize, const CommandManager& commandManager, wstring name)
{
	if (!CreateSwapChain(device, hwnd, winSize, commandManager, name)) {
		return false;
	}

	if (!CreateDepthStencilBuffer(device, winSize, name)) {
		return false;
	}

	return true;
}

ComPtr<IDXGISwapChain4> SwapChain::GetSwapChain()
{
	return swapChain_;
}

ComPtr<ID3D12Resource> SwapChain::GetCurrentBackBuffer(UINT index)
{
	return backBuffers_[index];
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