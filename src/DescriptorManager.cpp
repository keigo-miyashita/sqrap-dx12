#include "DescriptorManager.hpp"
#include "Device.hpp"
#include "Resource.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	DescriptorManager::DescriptorManager(const Device& device, std::initializer_list<DescriptorManagerDesc> descManagerDesc, DescriptorLayout descLayout)
		: pDevice_(&device)
	{
		if (descManagerDesc.size() == 0) {
			throw runtime_error("[DescriptorManager::DescriptorManager] descManagerDesc is empty");
		}

		if (descManagerDesc.size() != descLayout.GetDescLayoutDesc().size()) {
			throw runtime_error("[DescriptorManager::DescriptorManager] descManagerDesc size does not match descLayout size");
		}

		UINT resourceNum = 0;
		UINT samplerNum = 0;
		for (auto& desc : descManagerDesc) {
			if (desc.type_ == ViewType::SAMPLER) {
				samplerNum++;
				hasSampler_ = true;
			}
			else {
				resourceNum++;
				hasResource_ = true;
			}
		}

		const UINT resourceStart = pDevice_->AllocateResourceDescriptorFrontIndex(resourceNum);
		const UINT samplerStart = pDevice_->AllocateSamplerDescriptorFrontIndex(samplerNum);

		resourceDescHeapGPUHandle_ = CD3DX12_GPU_DESCRIPTOR_HANDLE(
			pDevice_->GetResourceDescriptorHeap()->GetGPUDescriptorHandleForHeapStart(),
			resourceStart,
			pDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
		);
		samplerDescHeapGPUHandle_ = CD3DX12_GPU_DESCRIPTOR_HANDLE(
			pDevice_->GetSamplerDescriptorHeap()->GetGPUDescriptorHandleForHeapStart(),
			samplerStart,
			pDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
		);

		UINT resourceViewOffset = resourceStart;
		UINT samplerViewOffset = samplerStart;
		
		auto& descLayoutDesc = descLayout.GetDescLayoutDesc();
		for (int i = 0; i < descManagerDesc.size(); i++) {
			const auto& desc = descManagerDesc.begin()[i];
			if ((desc.type_ != descLayoutDesc[i].first)
				|| (desc.regIndex_ != descLayoutDesc[i].second)) {
				throw runtime_error("[DescriptorManager::DescriptorManager] descriptor " + to_string(i) + " does not match the layout");
			}
			if (desc.type_ == ViewType::CBV) {
				auto viewDesc = desc.resource_->MakeCBV();
				auto heapHandle = pDevice_->GetResourceDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
				heapHandle.ptr += pDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * resourceViewOffset;
				pDevice_->GetDevice()->CreateConstantBufferView(&viewDesc, heapHandle);
				resourceViewOffset++;
			}
			else if (desc.type_ == ViewType::SRV) {
				auto viewDesc = desc.resource_->MakeSRV();
				auto heapHandle = pDevice_->GetResourceDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
				heapHandle.ptr += pDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * resourceViewOffset;
				auto resource = desc.resource_->GetResource().Get();
				if (desc.resource_->GetResourceType() == ResourceType::AS) {
					resource = nullptr;
				}
				pDevice_->GetDevice()->CreateShaderResourceView(resource, &viewDesc, heapHandle);
				resourceViewOffset++;
			}
			else if (desc.type_ == ViewType::UAV) {
				auto viewDesc = desc.resource_->MakeUAV(desc.isCounter_, desc.mipSlice_);
				auto heapHandle = pDevice_->GetResourceDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
				heapHandle.ptr += pDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * resourceViewOffset;
				auto counterBuffer = desc.isCounter_ ? desc.resource_->GetResource().Get() : nullptr;
				pDevice_->GetDevice()->CreateUnorderedAccessView(desc.resource_->GetResource().Get(), counterBuffer, &viewDesc, heapHandle);
				resourceViewOffset++;
			}
			else if (desc.type_ == ViewType::SAMPLER) {
				auto viewDesc = desc.samplerDesc_;
				auto heapHandle = pDevice_->GetSamplerDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
				heapHandle.ptr += pDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) * samplerViewOffset;
				pDevice_->GetDevice()->CreateSampler(&viewDesc, heapHandle);
				samplerViewOffset++;
			}
		}
	}

	bool DescriptorManager::HasResource() const
	{
		return hasResource_;
	}

	bool DescriptorManager::HasSampler() const
	{
		return hasSampler_;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::GetResourceDescHeapGPUHandle() const
	{
		return resourceDescHeapGPUHandle_;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::GetSamplerDescHeapGPUHandle() const
	{
		return samplerDescHeapGPUHandle_;
	}
}