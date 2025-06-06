#include "Gui.hpp"

#include "Command.hpp"
#include "Device.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

bool GUI::InitializeGUI(const HWND& hwnd)
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 1;
	auto result = pDevice_->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(imguiDescHeap_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		throw std::runtime_error("Failed to CreateDescriptorHeap for GUI : " + to_string(result));
		return false;
	}

	if (ImGui::CreateContext() == nullptr) {
		throw std::runtime_error("Failed to ImGui::CreateContext : " + to_string(result));
		return false;
	}

	bool binResult = ImGui_ImplWin32_Init(hwnd);
	if (!binResult) {
		throw std::runtime_error("Failed to ImGui_ImplWin32_Init");
		return false;
	}

	auto imguiCPUHandle = imguiDescHeap_->GetCPUDescriptorHandleForHeapStart();
	auto imguiGPUHandle = imguiDescHeap_->GetGPUDescriptorHandleForHeapStart();
	binResult = ImGui_ImplDX12_Init(pDevice_->GetDevice().Get(), 2, DXGI_FORMAT_R8G8B8A8_UNORM, imguiDescHeap_.Get(), imguiCPUHandle, imguiGPUHandle);
	if (!binResult) {
		throw std::runtime_error("Failed to ImGui_ImplDX12_Init");
		return false;
	}

	return true;
}

GUI::GUI(const Device& device, const HWND& hwnd)
	: pDevice_(&device)
{
	InitializeGUI(hwnd);
}

GUI::~GUI()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

//bool GUI::Init(Device* pDevice, const HWND& hwnd)
//{
//	pDevice_ = pDevice;
//	if (pDevice == nullptr) {
//		cerr << "Class GUI does not have device" << endl;
//		return false;
//	}
//
//	if (!InitializeGUI(hwnd)) {
//		cerr << "Failed to InitializeGUI" << endl;
//		return false;
//	}
//
//	return true;
//}

void GUI::BeginCommand()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("GUI");
	ImGui::SetWindowSize(ImVec2(400, 500), ImGuiCond_::ImGuiCond_FirstUseEver);

}

void GUI::EndCommand()
{
	ImGui::End();

	ImGui::Render();
}

void GUI::Draw(Command& command)
{
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), command.GetCommandList().Get());
}

ComPtr<ID3D12DescriptorHeap> GUI::GetImguiDescHeap() const
{
	return imguiDescHeap_;
}