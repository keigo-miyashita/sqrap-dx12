#include "DescriptorLayout.hpp"
#include "Resource.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	DescriptorLayout::DescriptorLayout(std::vector<std::pair<ViewType, UINT>> descLayoutDesc, D3D12_DESCRIPTOR_RANGE_FLAGS flags)
		: descLayoutDesc_(descLayoutDesc)
	{
		if (descLayoutDesc.size() == 0) {
			throw runtime_error("[DescriptorLayout::DescriptorLayout] descLayoutDesc is empty");
		}

		D3D12_DESCRIPTOR_RANGE_TYPE previousType = ConvertDescriptorRangeType(descLayoutDesc.begin()->first);
		UINT baseReg = descLayoutDesc.begin()->second;
		UINT numReg = 0;
		for (auto& desc : descLayoutDesc) {
			auto currentType = ConvertDescriptorRangeType(desc.first);
			auto currentIndex = desc.second;
			if (!usedRegs_.insert({ desc.first, currentIndex }).second)
			{
				throw runtime_error("[DescriptorLayout::DescriptorLayout] duplicate register index for the same view type");
			}
			if (numReg > 0 && (currentType != previousType || currentIndex != baseReg + numReg)) {
				CD3DX12_DESCRIPTOR_RANGE1 descRange = {};
				descRange.Init(previousType, numReg, baseReg, 0, flags, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
				if (previousType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) {
					samplerDescRanges_.push_back(descRange);
				}
				else {
					resourceDescRanges_.push_back(descRange);
				}
				baseReg = currentIndex;
				numReg = 0;
			}
			previousType = currentType;
			numReg++;
		}
		CD3DX12_DESCRIPTOR_RANGE1 descRange;
		descRange.Init(previousType, numReg, baseReg, 0, flags, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
		if (previousType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) {
			samplerDescRanges_.push_back(descRange);
		}
		else {
			resourceDescRanges_.push_back(descRange);
		}
	}

	D3D12_DESCRIPTOR_RANGE_TYPE DescriptorLayout::ConvertDescriptorRangeType(ViewType type)
	{
		if (type == ViewType::CBV) {
			return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		}
		else if (type == ViewType::SRV) {
			return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		}
		else if (type == ViewType::UAV) {
			return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		}
		else if (type == ViewType::SAMPLER) {
			return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		}
		else {
			throw runtime_error("[DescriptorLayout::ConvertDescriptorRangeType] Invalid view type");
		}
	}

	UINT DescriptorLayout::GetResourceDescNum() const
	{
		return static_cast<UINT>(resourceDescRanges_.size());
	}

	UINT DescriptorLayout::GetSamplerDescNum() const
	{
		return static_cast<UINT>(samplerDescRanges_.size());
	}

	const CD3DX12_DESCRIPTOR_RANGE1* DescriptorLayout::GetPResourceDescRanges() const
	{
		return resourceDescRanges_.data();
	}

	const CD3DX12_DESCRIPTOR_RANGE1* DescriptorLayout::GetPSamplerDescRanges() const
	{
		return samplerDescRanges_.data();
	}

	const std::set<std::pair<ViewType, UINT>>& DescriptorLayout::GetUsedRegs() const
	{
		return usedRegs_;
	}

	const std::vector<std::pair<ViewType, UINT>>& DescriptorLayout::GetDescLayoutDesc() const
	{
		return descLayoutDesc_;
	}
}