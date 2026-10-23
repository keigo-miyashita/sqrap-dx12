#pragma once

#include "pch.hpp"

#include "Alias.hpp"

namespace sqrp
{
	class Buffer;
	class Device;
	class Resource;
	class Texture;

	enum class ViewType
	{
		NONE, CBV, SRV, UAV, SAMPLER,
	};

	class DescriptorLayout
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		// Descriptor Table
		std::vector<CD3DX12_DESCRIPTOR_RANGE1> resourceDescRanges_;
		std::vector<CD3DX12_DESCRIPTOR_RANGE1> samplerDescRanges_;
		std::set<std::pair<ViewType, UINT>> usedRegs_;
		std::vector<std::pair<ViewType, UINT>> descLayoutDesc_;

	public:
		DescriptorLayout(std::vector<std::pair<ViewType, UINT>> descLayoutDesc, D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE);

		static D3D12_DESCRIPTOR_RANGE_TYPE ConvertDescriptorRangeType(ViewType type);
		UINT GetResourceDescNum() const;
		UINT GetSamplerDescNum() const;
		const CD3DX12_DESCRIPTOR_RANGE1* GetPResourceDescRanges() const;
		const CD3DX12_DESCRIPTOR_RANGE1* GetPSamplerDescRanges() const;
		const std::set<std::pair<ViewType, UINT>>& GetUsedRegs() const;
		const std::vector<std::pair<ViewType, UINT>>& GetDescLayoutDesc() const;
	};
}
