#include <common.hpp>
#include <comdef.h>

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
	HRESULT result = pDevice_->GetDevice()->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &rscDesc, rscState_, nullptr, IID_PPV_ARGS(resource_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		_com_error err(result);
		wcerr << L"Error message: " << err.ErrorMessage() << endl;
		result = pDevice_->GetDevice()->GetDeviceRemovedReason();
		_com_error DeviceRemovedReason(result);
		wcerr << L"Device removed reason: " << DeviceRemovedReason.ErrorMessage() << endl;
		cerr << "Failed to CreateCommittedResource" << endl;
		return false;
	}
	resource_->SetName(name.c_str());
	return true;
}

bool Buffer::CreateCounterBuffer(UINT strideSize, UINT numElement, std::wstring name)
{
	heapType_ = D3D12_HEAP_TYPE_DEFAULT;
	rscFlag_ = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	strideSize_ = strideSize;
	numElement_ = numElement;
	rscState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	offsetCounter_ = AlignForUAVCounter(strideSize * numElement);
	auto heapProp = CD3DX12_HEAP_PROPERTIES(heapType_);
	auto rscDesc = CD3DX12_RESOURCE_DESC::Buffer(AlignForUAVCounter(strideSize_ * numElement_) + sizeof(UINT), rscFlag_);
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
	if (pDevice_ == nullptr) {
		cerr << "Buffer class pDevice doesn't have any pounter" << endl; ;
		return false;
	}
	if (!CreateBuffer(strideSize, numElement, heapType, rscFlag, initRscState, name)) {
		cerr << "Failed to create buffer" << endl;
		return false;
	}

	return true;
}

bool Buffer::InitAsCounter(Device* pDevice, UINT strideSize, UINT numElement, wstring name)
{
	pDevice_ = pDevice;
	if (!CreateCounterBuffer(strideSize, numElement, name)) {
		return false;
	}

	return true;
}

bool Buffer::InitAsUpload(Device* pDevice, UINT strideSize, UINT numElement, wstring name)
{
	pDevice_ = pDevice;
	if (!CreateBuffer(strideSize, numElement, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_SOURCE, name)) {
		cerr << "Failed to CreateBuffer" << endl;
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

void Buffer::Reset()
{
	resource_.Reset();
}

ComPtr<ID3D12Resource> Buffer::GetResource() const
{
	return resource_;
}

D3D12_GPU_VIRTUAL_ADDRESS Buffer::GetGPUAddress() const
{
	return resource_->GetGPUVirtualAddress();
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

UINT Buffer::GetOffsetCounter() const
{
	return offsetCounter_;
}

void Buffer::SetResourceState(D3D12_RESOURCE_STATES rscState)
{
	rscState_ = rscState;
}