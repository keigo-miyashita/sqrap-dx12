#pragma once

#include "pch.hpp"

namespace sqrp
{
	class Command;
	class Device;

	class GUI
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const Device* pDevice_ = nullptr;
		ComPtr<ID3D12DescriptorHeap> imguiDescHeap_ = nullptr;

		// slot 0: ImGui font texture (reserved)
		// slots 1..kMaxUserTextures: user textures (scene view etc.)
		static constexpr UINT kMaxUserTextures = 8;

		void InitializeGUI(const HWND& hwnd);

	public:
		GUI(const Device& device, const HWND& hwnd);
		~GUI();
		void BeginCommand();
		void EndCommand();
		void Draw(Command& command);
		ComPtr<ID3D12DescriptorHeap> GetImguiDescHeap() const;

		// Register (or update) a user texture at the given slot (1-based).
		// Returns the D3D12_GPU_DESCRIPTOR_HANDLE to pass as ImTextureID.
		// Safe to call multiple times on the same slot (e.g. after resize).
		D3D12_GPU_DESCRIPTOR_HANDLE RegisterTextureSRV(UINT slot,
		                                               ID3D12Resource* resource,
		                                               DXGI_FORMAT format);
	};
}