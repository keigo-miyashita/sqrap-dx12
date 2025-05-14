#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

bool GUI::InitializeGui(const Device& device, const HWND& hwnd)
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 1;
	auto result = device.GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(imguiDescHeap_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		return false;
	}

	if (ImGui::CreateContext() == nullptr) {
		cerr << "Failed to create imgui context\n";
		return false;
	}

	bool binResult = ImGui_ImplWin32_Init(hwnd);
	if (!binResult) {
		cerr << "Failed to initialize implWin32\n";
		return false;
	}

	auto imguiCPUHandle = imguiDescHeap_->GetCPUDescriptorHandleForHeapStart();
	auto imguiGPUHandle = imguiDescHeap_->GetGPUDescriptorHandleForHeapStart();
	binResult = ImGui_ImplDX12_Init(device.GetDevice().Get(), 2, DXGI_FORMAT_R8G8B8A8_UNORM, imguiDescHeap_.Get(), imguiCPUHandle, imguiGPUHandle);
	if (!binResult) {
		cerr << "Failed to initialize implDX112\n";
		return false;
	}

	return true;
}

GUI::GUI()
{

}

bool GUI::Init()
{
	return true;
}

ComPtr<ID3D12DescriptorHeap> GUI::GetImguiDescHeap()
{
	return imguiDescHeap_;
}