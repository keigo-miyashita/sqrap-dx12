#include "Descriptormanager.hpp"
#include "Device.hpp"
#include "Resource.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	void DescriptorManager::CreateCBV(ResourceHandle resource)
	{
		resource->CreateCBV(*this, viewOffset_);
		viewOffset_++;
	}

	void DescriptorManager::CreateSRV(ResourceHandle resource)
	{
		resource->CreateSRV(*this, viewOffset_);
		viewOffset_++;
	}

	void DescriptorManager::CreateUAV(ResourceHandle resource)
	{
		resource->CreateUAV(*this, viewOffset_);
		viewOffset_++;
	}

	void DescriptorManager::CreateUAVCounter(ResourceHandle resource)
	{
		resource->CreateUAVCounter(*this, viewOffset_);
		viewOffset_++;
	}

	void DescriptorManager::CreateSampler()
	{
		viewOffset_++;
	}

	DescriptorManager::DescriptorManager(const Device& device, HeapType heapType, std::initializer_list<DescriptorManagerDesc> descManagerDesc, D3D12_DESCRIPTOR_RANGE_FLAGS flags, std::wstring name)
		: pDevice_(&device), heapType_(heapType), name_(name)
	{
		numDescriptor_ = descManagerDesc.size();

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		if (heapType_ == HeapType::Resource) {
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		}
		else if (heapType_ == HeapType::Sampler) {
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		}
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.NodeMask = 0;
		heapDesc.NumDescriptors = numDescriptor_;
		HRESULT result = pDevice_->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(descHeap_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw std::runtime_error("Failed to CreateDescriptorHeap : " + to_string(result));
		}
		descHeap_->SetName(name_.c_str());

		ViewType currentType = ViewType::NONE;
		for (auto desc : descManagerDesc) {
			if (desc.type_ != currentType)
			{
				currentType = desc.type_;
				switch (currentType)
				{
				case ViewType::CBV:    baseRegCBV_ = desc.numReg_; break;
				case ViewType::SRV:    baseRegSRV_ = desc.numReg_; break;
				case ViewType::UAV:    baseRegUAV_ = desc.numReg_; break;
				case ViewType::SAMPLER: baseRegSampler_ = desc.numReg_; break;
				default: break;
				}
			}
			switch (desc.type_)
			{
			case ViewType::CBV:
				numCBV_++;
				CreateCBV(desc.resource_);
				break;
			case ViewType::SRV:
				numSRV_++;
				CreateSRV(desc.resource_);
				break;
			case ViewType::UAV:
				numUAV_++;
				if (desc.isCounter_) {
					CreateUAVCounter(desc.resource_);
				}
				else {
					CreateUAV(desc.resource_);
				}
				break;
			case ViewType::SAMPLER:
				numSampler_++;
				CreateSampler();
				break;
			default:
				break;
			}
		}

		if (heapType_ == HeapType::Resource) {
			if (numCBV_ != 0) {
				CD3DX12_DESCRIPTOR_RANGE1 descRange = {};
				descRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, numCBV_, baseRegCBV_, 0, flags, 0);
				descRanges_.push_back(descRange);
			}
			if (numSRV_ != 0) {
				CD3DX12_DESCRIPTOR_RANGE1 descRange = {};
				descRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, numSRV_, baseRegSRV_, 0, flags, numCBV_);
				descRanges_.push_back(descRange);
			}
			if (numUAV_ != 0) {
				CD3DX12_DESCRIPTOR_RANGE1 descRange = {};
				descRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, numUAV_, baseRegUAV_, 0, flags, numCBV_ + numSRV_);
				descRanges_.push_back(descRange);
			}
		}
		else if (heapType_ == HeapType::Sampler) {
			if (numSampler_ != 0) {
				CD3DX12_DESCRIPTOR_RANGE1 descRange = {};
				descRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, numSampler_, baseRegSampler_, 0, flags, 0);
				descRanges_.push_back(descRange);
			}
		}
	}

	ComPtr<ID3D12DescriptorHeap> DescriptorManager::GetDescriptorHeap() const
	{
		return descHeap_;
	}

	HeapType DescriptorManager::GetHeapType() const
	{
		return heapType_;
	}

	UINT DescriptorManager::GetNumDescRanges() const
	{
		return descRanges_.size();
	}

	const CD3DX12_DESCRIPTOR_RANGE1* DescriptorManager::GetPDescRanges() const
	{
		return descRanges_.data();
	}
}