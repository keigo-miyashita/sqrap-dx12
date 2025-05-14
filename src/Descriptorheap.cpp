#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

bool DescriptorHeap::CreateDescriptorHeap(const Device& device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptor, wstring name)
{
	heapType_ = heapType;
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	heapDesc.Type = heapType_;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = numDescriptor;
	if (FAILED(device.GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(descHeap_.ReleaseAndGetAddressOf())))) {
		return false;
	}
	descHeap_->SetName(name.c_str());
	return true;
}

DescriptorHeap::DescriptorHeap()
{

}

bool DescriptorHeap::Init(const Device& device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptor, wstring name)
{
	if (!CreateDescriptorHeap(device, heapType, numDescriptor, name)) {
		return false;
	}

	return true;
}

void DescriptorHeap::CreateCBV(const Device& device, const Buffer& buff, UINT viewOffset)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = {};
	viewDesc.BufferLocation = buff.GetResource()->GetGPUVirtualAddress();
	viewDesc.SizeInBytes = buff.GetStrideSize() * buff.GetNumElement();
	auto heapHandle = descHeap_->GetCPUDescriptorHandleForHeapStart();
	heapHandle.ptr += device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * viewOffset;
	device.GetDevice()->CreateConstantBufferView(&viewDesc, heapHandle);
}

void DescriptorHeap::CreateSRV(const Device& device, const Buffer& buff, UINT viewOffset)
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
	heapHandle.ptr += device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * viewOffset;
	device.GetDevice()->CreateShaderResourceView(buff.GetResource().Get(), & viewDesc, heapHandle);
}

void DescriptorHeap::CreateUAV(const Device& device, const Buffer& buff, UINT viewOffset)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
	viewDesc.Format = DXGI_FORMAT_UNKNOWN;
	viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	viewDesc.Buffer.StructureByteStride = buff.GetStrideSize();
	viewDesc.Buffer.NumElements = buff.GetNumElement();
	viewDesc.Buffer.FirstElement = 0;
	viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	auto heapHandle = descHeap_->GetCPUDescriptorHandleForHeapStart();
	heapHandle.ptr += device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * viewOffset;
	device.GetDevice()->CreateUnorderedAccessView(buff.GetResource().Get(), nullptr, &viewDesc, heapHandle);
}

void DescriptorHeap::CreateUAVCounter(const Device& device, const Buffer& buff, UINT viewOffset)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
	viewDesc.Format = DXGI_FORMAT_UNKNOWN;
	viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	viewDesc.Buffer.StructureByteStride = buff.GetStrideSize();
	viewDesc.Buffer.NumElements = buff.GetNumElement();
	viewDesc.Buffer.FirstElement = 0;
	viewDesc.Buffer.CounterOffsetInBytes = buff.GetStrideSize() * buff.GetNumElement();
	viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	auto heapHandle = descHeap_->GetCPUDescriptorHandleForHeapStart();
	heapHandle.ptr += device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * viewOffset;
	device.GetDevice()->CreateUnorderedAccessView(buff.GetResource().Get(), nullptr, &viewDesc, heapHandle);
}

void DescriptorHeap::CreateSampler(const Device& device, UINT viewOffset)
{

}

ComPtr<ID3D12DescriptorHeap> DescriptorHeap::GetDescriptorHeap()
{
	return descHeap_;
}

D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeap::GetHeapType()
{
	return heapType_;
}