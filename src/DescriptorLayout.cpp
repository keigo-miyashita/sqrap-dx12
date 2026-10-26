#include "DescriptorLayout.hpp"
#include "Resource.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	DescriptorLayout::DescriptorLayout(DescriptorLayoutType descLayoutType, std::vector<std::pair<ViewType, UINT>> descLayoutDesc, D3D12_DESCRIPTOR_RANGE_FLAGS flags)
		: descLayoutType_(descLayoutType), descLayoutDesc_(descLayoutDesc)
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
			if (descLayoutType_ == DescriptorLayoutType::Resource && desc.first == ViewType::SAMPLER) {
				throw runtime_error("[DescriptorLayout::DescriptorLayout] SAMPLER cannot be used in a Resource layout");
			}
			if (descLayoutType_ == DescriptorLayoutType::Sampler && desc.first != ViewType::SAMPLER) {
				throw runtime_error("[DescriptorLayout::DescriptorLayout] only SAMPLER can be used in a Sampler layout");
			}
			if (!usedRegs_.insert({ desc.first, currentIndex }).second)
			{
				throw runtime_error("[DescriptorLayout::DescriptorLayout] duplicate register index for the same view type");
			}
			if (numReg > 0 && (currentType != previousType || currentIndex != baseReg + numReg)) {
				CD3DX12_DESCRIPTOR_RANGE1 descRange = {};
				descRange.Init(previousType, numReg, baseReg, 0, flags, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
				descRanges_.push_back(descRange);
				baseReg = currentIndex;
				numReg = 0;
			}
			previousType = currentType;
			numReg++;
		}
		CD3DX12_DESCRIPTOR_RANGE1 descRange;
		descRange.Init(previousType, numReg, baseReg, 0, flags, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
		descRanges_.push_back(descRange);
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
			throw runtime_error("[DescriptorLayout::ConvertDescriptorRangeType] invalid view type");
		}
	}

	UINT DescriptorLayout::GetRangeNum() const
	{
		return static_cast<UINT>(descRanges_.size());
	}

	UINT DescriptorLayout::GetViewNum() const
	{
		return static_cast<UINT>(descLayoutDesc_.size());
	}

	std::span<const CD3DX12_DESCRIPTOR_RANGE1> DescriptorLayout::GetDescRanges() const
	{
		return descRanges_;
	}

	const std::set<std::pair<ViewType, UINT>>& DescriptorLayout::GetUsedRegs() const
	{
		return usedRegs_;
	}

	const std::vector<std::pair<ViewType, UINT>>& DescriptorLayout::GetDescLayoutDesc() const
	{
		return descLayoutDesc_;
	}

	DescriptorLayoutType DescriptorLayout::GetDescriptorLayoutType() const
	{
		return descLayoutType_;
	}
}