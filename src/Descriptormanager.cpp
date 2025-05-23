#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

bool DescriptorManager::CreateDescriptorHeap(std::wstring name)
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	heapDesc.Type = heapType_;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = numDescriptor_;
	if (FAILED(pDevice_->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(descHeap_.ReleaseAndGetAddressOf())))) {
		return false;
	}
	descHeap_->SetName(name.c_str());
	return true;
}

bool DescriptorManager::CreateDescriptorTable()
{
	if (heapType_ == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
		if (numCBV_ != 0) {
			CD3DX12_DESCRIPTOR_RANGE descRange = {};
			descRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, numCBV_, baseRegCBV_, 0, 0);
			descRanges_.push_back(descRange);
		}
		if (numSRV_ != 0) {
			CD3DX12_DESCRIPTOR_RANGE descRange = {};
			descRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, numSRV_, baseRegSRV_, 0, numCBV_);
			descRanges_.push_back(descRange);
		}
		if (numUAV_ != 0) {
			CD3DX12_DESCRIPTOR_RANGE descRange = {};
			descRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, numUAV_, baseRegUAV_, 0, numCBV_ + numSRV_);
			descRanges_.push_back(descRange);
		}
	}
	else if (heapType_ == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
		if (numSampler_ != 0) {
			CD3DX12_DESCRIPTOR_RANGE descRange = {};
			descRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, numSampler_, baseRegSampler_, 0, 0);
			descRanges_.push_back(descRange);
		}
	}

	return true;
}

DescriptorManager::DescriptorManager()
{

}

bool DescriptorManager::InitAsBuffer(Device* pDevice, UINT baseRegCBV, UINT numCBV, UINT baseRegSRV, UINT numSRV, UINT baseRegUAV, UINT numUAV, std::wstring name)
{
	pDevice_ = pDevice;
	if (pDevice_ == nullptr) {
		cerr << "DescriptorManager class does't have Device class pointer" << endl;
		return false;
	}
	heapType_ = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	baseRegCBV_ = baseRegCBV;
	numCBV_ = numCBV;
	baseRegSRV_ = baseRegSRV;
	numSRV_ = numSRV;
	baseRegUAV_ = baseRegUAV;
	numUAV_ = numUAV;
	numDescriptor_ = numCBV_ + numSRV_ + numUAV_;
	if (!CreateDescriptorHeap(name)) {
		return false;
	}

	if (!CreateDescriptorTable()) {
		return false;
	}

	return true;
}

bool DescriptorManager::InitAsSampler(Device* pDevice, UINT baseRegSampler, UINT numSampler, std::wstring name)
{
	pDevice_ = pDevice;
	heapType_ = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	baseRegSampler_ = baseRegSampler;
	numSampler_ = numSampler;
	numDescriptor_ = numSampler_;
	if (!CreateDescriptorHeap(name)) {
		return false;
	}

	if (!CreateDescriptorTable()) {
		return false;
	}

	return true;
}

void DescriptorManager::CreateCBV(const Buffer& buff, UINT viewOffset)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = {};
	viewDesc.BufferLocation = buff.GetResource()->GetGPUVirtualAddress();
	viewDesc.SizeInBytes = buff.GetStrideSize() * buff.GetNumElement();
	auto heapHandle = descHeap_->GetCPUDescriptorHandleForHeapStart();
	heapHandle.ptr += pDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * viewOffset;
	pDevice_->GetDevice()->CreateConstantBufferView(&viewDesc, heapHandle);
}

void DescriptorManager::CreateSRV(const Buffer& buff, UINT viewOffset)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
	viewDesc.Format = DXGI_FORMAT_UNKNOWN;
	viewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	viewDesc.Buffer.StructureByteStride = buff.GetStrideSize();
	viewDesc.Buffer.NumElements = buff.GetNumElement();
	viewDesc.Buffer.FirstElement = 0;
	viewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	auto heapHandle = descHeap_->GetCPUDescriptorHandleForHeapStart();
	heapHandle.ptr += pDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * viewOffset;
	pDevice_->GetDevice()->CreateShaderResourceView(buff.GetResource().Get(), & viewDesc, heapHandle);
}

void DescriptorManager::CreateUAV(const Buffer& buff, UINT viewOffset)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
	viewDesc.Format = DXGI_FORMAT_UNKNOWN;
	viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	viewDesc.Buffer.StructureByteStride = buff.GetStrideSize();
	viewDesc.Buffer.NumElements = buff.GetNumElement();
	viewDesc.Buffer.FirstElement = 0;
	viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	auto heapHandle = descHeap_->GetCPUDescriptorHandleForHeapStart();
	heapHandle.ptr += pDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * viewOffset;
	pDevice_->GetDevice()->CreateUnorderedAccessView(buff.GetResource().Get(), nullptr, &viewDesc, heapHandle);
}

void DescriptorManager::CreateUAVCounter(const Buffer& buff, UINT viewOffset)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
	viewDesc.Format = DXGI_FORMAT_UNKNOWN;
	viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	viewDesc.Buffer.StructureByteStride = buff.GetStrideSize();
	viewDesc.Buffer.NumElements = buff.GetNumElement();
	viewDesc.Buffer.FirstElement = 0;
	viewDesc.Buffer.CounterOffsetInBytes = buff.GetOffsetCounter();
	viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	auto heapHandle = descHeap_->GetCPUDescriptorHandleForHeapStart();
	heapHandle.ptr += pDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * viewOffset;
	pDevice_->GetDevice()->CreateUnorderedAccessView(buff.GetResource().Get(), buff.GetResource().Get(), &viewDesc, heapHandle);
}

void DescriptorManager::CreateSampler(UINT viewOffset)
{

}

ComPtr<ID3D12DescriptorHeap> DescriptorManager::GetDescriptorHeap() const
{
	return descHeap_;
}

D3D12_DESCRIPTOR_HEAP_TYPE DescriptorManager::GetHeapType() const
{
	return heapType_;
}

UINT DescriptorManager::GetNumDescRanges() const
{
	return descRanges_.size();
}

const CD3DX12_DESCRIPTOR_RANGE* DescriptorManager::GetPDescRanges() const
{
	return descRanges_.data();
}