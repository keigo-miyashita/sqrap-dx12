#include "Gui.hpp"

#include "Command.hpp"
#include "Device.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	void Gui::InitializeGui(const HWND& hwnd)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.NodeMask = 0;
		heapDesc.NumDescriptors = 1 + kMaxUserTextures; // slot 0: font, slots 1+: user textures
		auto result = pDevice_->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(imguiDescHeap_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("Failed to CreateDescriptorHeap for Gui : " + to_string(result));
		}

		if (ImGui::CreateContext() == nullptr) {
			throw runtime_error("Failed to ImGui::CreateContext : " + to_string(result));
		}

		bool binResult = ImGui_ImplWin32_Init(hwnd);
		if (!binResult) {
			throw runtime_error("Failed to ImGui_ImplWin32_Init");
		}

		auto imguiCPUHandle = imguiDescHeap_->GetCPUDescriptorHandleForHeapStart();
		auto imguiGPUHandle = imguiDescHeap_->GetGPUDescriptorHandleForHeapStart();
		binResult = ImGui_ImplDX12_Init(pDevice_->GetDevice().Get(), 2, DXGI_FORMAT_R8G8B8A8_UNORM, imguiDescHeap_.Get(), imguiCPUHandle, imguiGPUHandle);
		if (!binResult) {
			throw runtime_error("Failed to ImGui_ImplDX12_Init");
		}
	}

	Gui::Gui(const Device& device, const HWND& hwnd)
		: pDevice_(&device)
	{
		InitializeGui(hwnd);
	}

	Gui::~Gui()
	{
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	void Gui::BeginCommand()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("GUI");
		ImGui::SetWindowSize(ImVec2(400, 500), ImGuiCond_::ImGuiCond_FirstUseEver);
	}

	void Gui::EndCommand()
	{
		ImGui::End();

		ImGui::Render();
	}

	void Gui::Draw(Command& command)
	{
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), command.GetCommandList().Get());
	}

	const ComPtr<ID3D12DescriptorHeap>& Gui::GetImGuiDescHeap() const
	{
		return imguiDescHeap_;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE Gui::RegisterTextureSRV(UINT slot,
	                                                     ID3D12Resource* resource,
	                                                     DXGI_FORMAT format)
	{
		if (slot == 0 || slot > kMaxUserTextures)
			throw runtime_error("Gui::RegisterTextureSRV: slot must be 1.." + to_string(kMaxUserTextures));

		UINT inc = pDevice_->GetDevice()->GetDescriptorHandleIncrementSize(
		               D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_CPU_DESCRIPTOR_HANDLE cpu = imguiDescHeap_->GetCPUDescriptorHandleForHeapStart();
		cpu.ptr += slot * inc;

		D3D12_GPU_DESCRIPTOR_HANDLE gpu = imguiDescHeap_->GetGPUDescriptorHandleForHeapStart();
		gpu.ptr += slot * inc;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format                  = format;
		srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels     = 1;

		pDevice_->GetDevice()->CreateShaderResourceView(resource, &srvDesc, cpu);
		return gpu;
	}
}