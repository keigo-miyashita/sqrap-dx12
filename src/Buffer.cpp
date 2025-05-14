#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

bool Buffer::CreateBuffer(const Device& device, UINT strideSize, UINT numElement, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_FLAGS rscFlag, D3D12_RESOURCE_STATES initRscState, wstring name)
{
	heapType_ = heapType;
	rscFlag_ = rscFlag;
	strideSize_ = strideSize;
	numElement_ = numElement;
	initRscState_ = initRscState;
	auto heapProp = CD3DX12_HEAP_PROPERTIES(heapType);
	auto rscDesc = CD3DX12_RESOURCE_DESC::Buffer(strideSize_ * numElement_, rscFlag_);
	if (FAILED(device.GetDevice()->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &rscDesc, initRscState_, nullptr, IID_PPV_ARGS(resource_.ReleaseAndGetAddressOf())))) {
		return false;
	}
	resource_->SetName(name.c_str());
	return true;
}

UINT Buffer::AlignForUAVCounter(UINT size)
{
	const UINT alignment = D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT;
	return (size + (alignment - 1)) & ~(alignment - 1);
}

UINT Buffer::AlignForConstantBuffer(UINT size)
{
	return (size + 0xff) & ~0xff;
}

Buffer::Buffer()
{

}

bool Buffer::Init(const Device& device, UINT strideSize, UINT numElement, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_FLAGS rscFlag, D3D12_RESOURCE_STATES initRscState, wstring name)
{
	if (!CreateBuffer(device, strideSize, numElement, heapType, rscFlag, initRscState, name)) {
		return false;
	}

	return true;
}

bool Buffer::InitAsUpload(const Device& device, UINT strideSize, UINT numElement, wstring name)
{
	if (!CreateBuffer(device, strideSize, numElement, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_SOURCE, name)) {
		return false;
	}

	return true;
}

bool Buffer::InitAsReadback(const Device& device, UINT strideSize, UINT numElement, wstring name)
{
	if (!CreateBuffer(device, strideSize, numElement, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, name)) {
		return false;
	}

	return true;
}

void* Buffer::Map()
{
	void* pData = nullptr;
	if (FAILED(resource_->Map(0, nullptr, &pData))) {
		cerr << "Failed to map buffer\n";
		return nullptr;
	}
	return pData;
}

void Buffer::Unmap()
{
	resource_->Unmap(0, nullptr);
}

void Buffer::Upload()
{

}

ComPtr<ID3D12Resource> Buffer::GetResource() const
{
	return resource_;
}

D3D12_HEAP_TYPE Buffer::GetHeapType() const
{
	return heapType_;
}

D3D12_RESOURCE_FLAGS Buffer::GetResourceFlag() const
{
	return rscFlag_;
}

UINT Buffer::GetStrideSize() const
{
	return strideSize_;
}

UINT Buffer::GetNumElement() const
{
	return numElement_;
}