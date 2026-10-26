#include "DescriptorManager.hpp"
#include "Device.hpp"
#include "Resource.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	UINT DescriptorManager::AllocateDescriptors(const Device& device, const DescriptorLayout& descLayout, size_t numDescs)
	{
		if (numDescs == 0) {
			throw runtime_error("[DescriptorManager::DescriptorManager] the descriptor list is empty");
		}

		if (numDescs != descLayout.GetDescLayoutDesc().size()) {
			throw runtime_error("[DescriptorManager::DescriptorManager] the number of descriptors does not match descLayout size");
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

		return heapStart;
	}

	DescriptorManager::DescriptorManager(const Device& device, std::initializer_list<DescriptorManagerDesc> descManagerDesc, DescriptorLayout descLayout)
	{
		if (descLayout.GetDescriptorLayoutType() != DescriptorLayoutType::Resource) {
			throw runtime_error("[DescriptorManager::DescriptorManager] a sampler layout must be created from D3D12_SAMPLER_DESC");
		}

		UINT viewOffset = AllocateDescriptors(device, descLayout, descManagerDesc.size());

		auto& descLayoutDesc = descLayout.GetDescLayoutDesc();
		for (int i = 0; i < descLayoutDesc.size(); i++) {
			const auto& layoutDesc = descLayoutDesc.begin()[i];
			const auto& managerDesc = descManagerDesc.begin()[i];
			if (!managerDesc.resource_) {
				throw runtime_error("[DescriptorManager::DescriptorManager] resource_ of descriptor " + to_string(i) + " is nullptr");
			}
			string reason;
			if (!managerDesc.resource_->CanCreateView(layoutDesc.first, &reason)) {
				throw runtime_error("[DescriptorManager::DescriptorManager] descriptor " + to_string(i) + ": " + reason);
			}
			// isCounter_ and mipSlice_ only mean something for a UAV (mipSlice_ only for a texture UAV)
			if (layoutDesc.first != ViewType::UAV && managerDesc.isCounter_) {
				throw runtime_error("[DescriptorManager::DescriptorManager] isCounter_ of descriptor " + to_string(i) + " is set, but it is not a UAV");
			}
			if (managerDesc.mipSlice_ != 0 && (layoutDesc.first != ViewType::UAV || managerDesc.resource_->GetResourceType() != ResourceType::Texture)) {
				throw runtime_error("[DescriptorManager::DescriptorManager] mipSlice_ of descriptor " + to_string(i) + " is set, but it is not a texture UAV");
			}
			auto heapHandle = device.GetResourceDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
			heapHandle.ptr += device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * viewOffset;
			if (layoutDesc.first == ViewType::CBV) {
				auto viewDesc = managerDesc.resource_->MakeCBV();
				device.GetDevice()->CreateConstantBufferView(&viewDesc, heapHandle);
				viewOffset++;
			}
			else if (layoutDesc.first == ViewType::SRV) {
				auto viewDesc = managerDesc.resource_->MakeSRV();
				auto resource = managerDesc.resource_->GetResource().Get();
				if (managerDesc.resource_->GetResourceType() == ResourceType::AS) {
					resource = nullptr;
				}
				device.GetDevice()->CreateShaderResourceView(resource, &viewDesc, heapHandle);
				viewOffset++;
			}
			else if (layoutDesc.first == ViewType::UAV) {
				auto viewDesc = managerDesc.resource_->MakeUAV(managerDesc.isCounter_, managerDesc.mipSlice_);
				auto counterBuffer = managerDesc.isCounter_ ? managerDesc.resource_->GetResource().Get() : nullptr;
				device.GetDevice()->CreateUnorderedAccessView(managerDesc.resource_->GetResource().Get(), counterBuffer, &viewDesc, heapHandle);
				viewOffset++;
			}
			else if (layoutDesc.first == ViewType::SAMPLER) {
				throw runtime_error("[DescriptorManager::DescriptorManager] a sampler cannot be created in the resource descriptor heap");
			}
		}
	}

	DescriptorManager::DescriptorManager(const Device& device, std::initializer_list<D3D12_SAMPLER_DESC> samplerDescs, DescriptorLayout descLayout)
	{
		if (descLayout.GetDescriptorLayoutType() != DescriptorLayoutType::Sampler) {
			throw runtime_error("[DescriptorManager::DescriptorManager] D3D12_SAMPLER_DESC can only be used with a sampler layout");
		}

		UINT viewOffset = AllocateDescriptors(device, descLayout, samplerDescs.size());

		auto& descLayoutDesc = descLayout.GetDescLayoutDesc();
		for (int i = 0; i < descLayoutDesc.size(); i++) {
			const auto& layoutDesc = descLayoutDesc.begin()[i];
			const auto& samplerDesc = samplerDescs.begin()[i];
			if (layoutDesc.first != ViewType::SAMPLER) {
				throw runtime_error("[DescriptorManager::DescriptorManager] a CBV, SRV or UAV cannot be created in the sampler descriptor heap");
			}
			auto heapHandle = device.GetSamplerDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
			heapHandle.ptr += device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) * viewOffset;
			device.GetDevice()->CreateSampler(&samplerDesc, heapHandle);
			viewOffset++;
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
