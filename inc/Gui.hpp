#pragma once

#include "pch.hpp"

namespace sqrp
{
	class Command;
	class Device;

	class Gui
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const Device* pDevice_ = nullptr;
		ComPtr<ID3D12DescriptorHeap> imguiDescHeap_ = nullptr;

		// slot 0: ImGui font texture (reserved)
		// slots 1..kMaxUserTextures: user textures (scene view etc.)
		static constexpr UINT kMaxUserTextures = 8;

		void InitializeGui(const HWND& hwnd);

	public:
		Gui(const Device& device, const HWND& hwnd);
		~Gui();
		Gui(const Gui&) = delete;
		Gui& operator=(const Gui&) = delete;
		void BeginCommand();
		void EndCommand();
		void Draw(Command& command);
		const ComPtr<ID3D12DescriptorHeap>& GetImGuiDescHeap() const;

		// Register (or update) a user texture at the given slot (1-based).
		// Returns the D3D12_GPU_DESCRIPTOR_HANDLE to pass as ImTextureID.
		// Safe to call multiple times on the same slot (e.g. after resize).
		D3D12_GPU_DESCRIPTOR_HANDLE RegisterTextureSRV(UINT slot,
		                                               ID3D12Resource* resource,
		                                               DXGI_FORMAT format);
	};
}