#include "DescriptorManager.hpp"
#include "Device.hpp"
#include "Resource.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	DescriptorManager::DescriptorManager(const Device& device, std::initializer_list<DescriptorManagerDesc> descManagerDesc, DescriptorLayout descLayout)
	{
		if (descManagerDesc.size() == 0) {
			throw runtime_error("[DescriptorManager::DescriptorManager] descManagerDesc is empty");
		}

		if (descManagerDesc.size() != descLayout.GetDescLayoutDesc().size()) {
			throw runtime_error("[DescriptorManager::DescriptorManager] descManagerDesc size does not match descLayout size");
		}

		descLayoutType_ = descLayout.GetDescriptorLayoutType();

		UINT heapOffsetNum = descLayout.GetViewNum();

		UINT heapStart = 0;
		D3D12_GPU_DESCRIPTOR_HANDLE heapGPUHandle = {};
		UINT incrementSize = 0;
		if (descLayoutType_ == DescriptorLayoutType::Resource) {
			heapStart = device.AllocateResourceDescriptorFrontIndex(heapOffsetNum);
			heapGPUHandle = device.GetResourceDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
			incrementSize = device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		else if (descLayoutType_ == DescriptorLayoutType::Sampler) {
			heapStart = device.AllocateSamplerDescriptorFrontIndex(heapOffsetNum);
			heapGPUHandle = device.GetSamplerDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
			incrementSize = device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		}

		descHeapGPUHandle_ = CD3DX12_GPU_DESCRIPTOR_HANDLE(heapGPUHandle, heapStart, incrementSize);

		UINT viewOffset = heapStart;
		
		auto& descLayoutDesc = descLayout.GetDescLayoutDesc();
		for (int i = 0; i < descLayoutDesc.size(); i++) {
			const auto& layoutDesc = descLayoutDesc.begin()[i];
			const auto& managerDesc = descManagerDesc.begin()[i];
			if (managerDesc.regIndex_ != layoutDesc.second) {
				throw runtime_error("[DescriptorManager::DescriptorManager] regIndex_ of descriptor " + to_string(i) + " does not match the layout");
			}
			if (layoutDesc.first == ViewType::CBV) {
				if (descLayoutType_ == DescriptorLayoutType::Sampler) {
					throw runtime_error("[DescriptorManager::DescriptorManager] a CBV cannot be created in the sampler descriptor heap");
				}
				auto viewDesc = managerDesc.resource_->MakeCBV();
				auto heapHandle = device.GetResourceDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
				heapHandle.ptr += device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * viewOffset;
				device.GetDevice()->CreateConstantBufferView(&viewDesc, heapHandle);
				viewOffset++;
			}
			else if (layoutDesc.first == ViewType::SRV) {
				if (descLayoutType_ == DescriptorLayoutType::Sampler) {
					throw runtime_error("[DescriptorManager::DescriptorManager] an SRV cannot be created in the sampler descriptor heap");
				}
				auto viewDesc = managerDesc.resource_->MakeSRV();
				auto heapHandle = device.GetResourceDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
				heapHandle.ptr += device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * viewOffset;
				auto resource = managerDesc.resource_->GetResource().Get();
				if (managerDesc.resource_->GetResourceType() == ResourceType::AS) {
					resource = nullptr;
				}
				device.GetDevice()->CreateShaderResourceView(resource, &viewDesc, heapHandle);
				viewOffset++;
			}
			else if (layoutDesc.first == ViewType::UAV) {
				if (descLayoutType_ == DescriptorLayoutType::Sampler) {
					throw runtime_error("[DescriptorManager::DescriptorManager] a UAV cannot be created in the sampler descriptor heap");
				}
				auto viewDesc = managerDesc.resource_->MakeUAV(managerDesc.isCounter_, managerDesc.mipSlice_);
				auto heapHandle = device.GetResourceDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
				heapHandle.ptr += device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * viewOffset;
				auto counterBuffer = managerDesc.isCounter_ ? managerDesc.resource_->GetResource().Get() : nullptr;
				device.GetDevice()->CreateUnorderedAccessView(managerDesc.resource_->GetResource().Get(), counterBuffer, &viewDesc, heapHandle);
				viewOffset++;
			}
			else if (layoutDesc.first == ViewType::SAMPLER) {
				if (descLayoutType_ == DescriptorLayoutType::Resource) {
					throw runtime_error("[DescriptorManager::DescriptorManager] a sampler cannot be created in the resource descriptor heap");
				}
				auto viewDesc = managerDesc.samplerDesc_;
				auto heapHandle = device.GetSamplerDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
				heapHandle.ptr += device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) * viewOffset;
				device.GetDevice()->CreateSampler(&viewDesc, heapHandle);
				viewOffset++;
			}
		}
	}

	DescriptorLayoutType DescriptorManager::GetDescriptorLayoutType() const
	{
		return descLayoutType_;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::GetDescHeapGPUHandle() const
	{
		return descHeapGPUHandle_;
	}
}