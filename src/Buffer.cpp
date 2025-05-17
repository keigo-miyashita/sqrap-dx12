#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

bool Buffer::CreateBuffer(UINT strideSize, UINT numElement, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_FLAGS rscFlag, D3D12_RESOURCE_STATES initRscState, wstring name)
{
	heapType_ = heapType;
	rscFlag_ = rscFlag;
	strideSize_ = strideSize;
	numElement_ = numElement;
	rscState_ = initRscState;
	auto heapProp = CD3DX12_HEAP_PROPERTIES(heapType);
	auto rscDesc = CD3DX12_RESOURCE_DESC::Buffer(strideSize_ * numElement_, rscFlag_);
	if (FAILED(pDevice_->GetDevice()->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &rscDesc, rscState_, nullptr, IID_PPV_ARGS(resource_.ReleaseAndGetAddressOf())))) {
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

bool Buffer::Init(Device* pDevice, UINT strideSize, UINT numElement, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_FLAGS rscFlag, D3D12_RESOURCE_STATES initRscState, wstring name)
{
	pDevice_ = pDevice;
	if (!CreateBuffer(strideSize, numElement, heapType, rscFlag, initRscState, name)) {
		return false;
	}

	return true;
}

bool Buffer::InitAsUpload(Device* pDevice, UINT strideSize, UINT numElement, wstring name)
{
	pDevice_ = pDevice;
	if (!CreateBuffer(strideSize, numElement, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_SOURCE, name)) {
		return false;
	}

	return true;
}

bool Buffer::InitAsReadback(Device* pDevice, UINT strideSize, UINT numElement, wstring name)
{
	pDevice_ = pDevice;
	if (!CreateBuffer(strideSize, numElement, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, name)) {
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

D3D12_RESOURCE_STATES Buffer::GetResourceState() const
{
	return rscState_;
}

UINT Buffer::GetStrideSize() const
{
	return strideSize_;
}

UINT Buffer::GetNumElement() const
{
	return numElement_;
}

void Buffer::SetResourceState(D3D12_RESOURCE_STATES rscState)
{
	rscState_ = rscState;
}