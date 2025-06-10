#include "Resource.hpp"

#include "Descriptormanager.hpp"
#include "Device.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

Resource::Resource(const Device& device, ResourceType rscType, std::wstring name)
	: pDevice_(&device), rscType_(rscType), name_(name)
{

}

Resource::Resource()
{

}

void Resource::CreateCBV(DescriptorManager& descManager, UINT viewOffset)
{
	if (rscType_ == ResourceType::Texture) {
		throw std::runtime_error("Cannot create CBV for texture !");
	}
}

void Resource::CreateUAV(DescriptorManager& descManager, UINT viewOffset)
{
	if (rscType_ == ResourceType::AS) {
		throw std::runtime_error("Cannot create CBV for AS !");
	}
}

void Resource::CreateUAVCounter(DescriptorManager& descManager, UINT viewOffset)
{
	if (rscType_ == ResourceType::Texture) {
		throw std::runtime_error("Cannot create UAVCounter for texture !");
	}
}

D3D12_HEAP_TYPE Resource::GetHeapType() const
{
	return heapType_;
}

ResourceType Resource::GetResourceType() const
{
	return rscType_;
}

D3D12_RESOURCE_FLAGS Resource::GetResourceFlag() const
{
	return rscFlag_;
}

D3D12_RESOURCE_STATES Resource::GetResourceState() const
{
	return rscState_;
}

ComPtr<ID3D12Resource> Resource::GetResource() const
{
	return resource_;
}

D3D12_GPU_VIRTUAL_ADDRESS Resource::GetGPUAddress() const
{
	return resource_->GetGPUVirtualAddress();
}

void Resource::SetResourceState(D3D12_RESOURCE_STATES rscState)
{
	rscState_ = rscState;
}

bool Buffer::CreateBuffer()
{
	auto heapProp = CD3DX12_HEAP_PROPERTIES(heapType_);
	auto rscDesc = CD3DX12_RESOURCE_DESC::Buffer(strideSize_ * numElement_, rscFlag_);
	HRESULT result = pDevice_->GetDevice()->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &rscDesc, rscState_, nullptr, IID_PPV_ARGS(resource_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		throw std::runtime_error("Failed to create buffer : " + to_string(result));
	}
	resource_->SetName(name_.c_str());
	return true;
}

bool Buffer::CreateCounterBuffer()
{
	heapType_ = D3D12_HEAP_TYPE_DEFAULT;
	rscFlag_ = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	rscState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	offsetCounter_ = AlignForUAVCounter(strideSize_ * numElement_);
	auto heapProp = CD3DX12_HEAP_PROPERTIES(heapType_);
	auto rscDesc = CD3DX12_RESOURCE_DESC::Buffer(AlignForUAVCounter(strideSize_ * numElement_) + sizeof(UINT), rscFlag_);
	HRESULT result = pDevice_->GetDevice()->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &rscDesc, rscState_, nullptr, IID_PPV_ARGS(resource_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		throw std::runtime_error("Failed to create buffer : " + to_string(result));
	}
	resource_->SetName(name_.c_str());
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

Buffer::Buffer(const Device& device, BufferType type, UINT strideSize, UINT numElement, std::wstring name)
	: Resource(device, ResourceType::Buffer, name), type_(type), strideSize_(strideSize), numElement_(numElement)
{
	if (type == BufferType::Counter) {
		heapType_ = D3D12_HEAP_TYPE_DEFAULT;
		rscFlag_ = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		rscState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		CreateCounterBuffer();
	}
	else {
		if (type == BufferType::Default) {
			heapType_ = D3D12_HEAP_TYPE_DEFAULT;
			rscFlag_ = D3D12_RESOURCE_FLAG_NONE;
			rscState_ = D3D12_RESOURCE_STATE_COMMON;
		}
		else if (type == BufferType::Upload) {
			heapType_ = D3D12_HEAP_TYPE_UPLOAD;
			rscFlag_ = D3D12_RESOURCE_FLAG_NONE;
			rscState_ = D3D12_RESOURCE_STATE_COPY_SOURCE;
		}
		else if (type == BufferType::Read) {
			heapType_ = D3D12_HEAP_TYPE_READBACK;
			rscFlag_ = D3D12_RESOURCE_FLAG_NONE;
			rscState_ = D3D12_RESOURCE_STATE_COPY_DEST;
		}
		else if (type == BufferType::Unordered) {
			heapType_ = D3D12_HEAP_TYPE_DEFAULT;
			rscFlag_ = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			rscState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}
		else if (type == BufferType::AS) {
			heapType_ = D3D12_HEAP_TYPE_DEFAULT;
			rscFlag_ = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			rscState_ = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		}
		CreateBuffer();
	}
}

void* Buffer::Map()
{
	void* pData = nullptr;
	HRESULT result = resource_->Map(0, nullptr, &pData);
	if (FAILED(result)) {
		cerr << "Failed to map buffer" << endl;;
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

void Buffer::CreateCBV(DescriptorManager& descManager, UINT viewOffset)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = {};
	viewDesc.BufferLocation = resource_->GetGPUVirtualAddress();
	viewDesc.SizeInBytes = strideSize_ * numElement_;
	auto heapHandle = descManager.GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	heapHandle.ptr += pDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * viewOffset;
	pDevice_->GetDevice()->CreateConstantBufferView(&viewDesc, heapHandle);
}

void Buffer::CreateSRV(DescriptorManager& descManager, UINT viewOffset)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
	viewDesc.Format = DXGI_FORMAT_UNKNOWN;
	viewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	viewDesc.Buffer.StructureByteStride = strideSize_;
	viewDesc.Buffer.NumElements = numElement_;
	viewDesc.Buffer.FirstElement = 0;
	viewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	auto heapHandle = descManager.GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	heapHandle.ptr += pDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * viewOffset;
	pDevice_->GetDevice()->CreateShaderResourceView(resource_.Get(), &viewDesc, heapHandle);
}

void Buffer::CreateUAV(DescriptorManager& descManager, UINT viewOffset)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
	viewDesc.Format = DXGI_FORMAT_UNKNOWN;
	viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	viewDesc.Buffer.StructureByteStride = strideSize_;
	viewDesc.Buffer.NumElements = numElement_;
	viewDesc.Buffer.FirstElement = 0;
	viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	auto heapHandle = descManager.GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	heapHandle.ptr += pDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * viewOffset;
	pDevice_->GetDevice()->CreateUnorderedAccessView(resource_.Get(), nullptr, &viewDesc, heapHandle);
}

void Buffer::CreateUAVCounter(DescriptorManager& descManager, UINT viewOffset)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
	viewDesc.Format = DXGI_FORMAT_UNKNOWN;
	viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	viewDesc.Buffer.StructureByteStride = strideSize_;
	viewDesc.Buffer.NumElements = numElement_;
	viewDesc.Buffer.FirstElement = 0;
	viewDesc.Buffer.CounterOffsetInBytes = offsetCounter_;
	viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	auto heapHandle = descManager.GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	heapHandle.ptr += pDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * viewOffset;
	pDevice_->GetDevice()->CreateUnorderedAccessView(resource_.Get(), resource_.Get(), &viewDesc, heapHandle);
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

Texture::Texture(const Device& device, TextureDim texDim, TextureType type, UINT strideSize, DXGI_FORMAT format, UINT width, UINT height, UINT depth, std::wstring name)
	: Resource(device, ResourceType::Texture, name), format_(format), texDim_(texDim), type_(type), strideSize_(strideSize), width_(width), height_(height), depth_(depth)
{
	if (type_ == TextureType::Default) {
		heapType_ = D3D12_HEAP_TYPE_DEFAULT;
		rscFlag_ = D3D12_RESOURCE_FLAG_NONE;
		rscState_ = D3D12_RESOURCE_STATE_COMMON;
	}
	else if (type_ == TextureType::Upload) {
		heapType_ = D3D12_HEAP_TYPE_UPLOAD;
		rscFlag_ = D3D12_RESOURCE_FLAG_NONE;
		rscState_ = D3D12_RESOURCE_STATE_COPY_SOURCE;
	}
	else if (type_ == TextureType::Read) {
		heapType_ = D3D12_HEAP_TYPE_READBACK;
		rscFlag_ = D3D12_RESOURCE_FLAG_NONE;
		rscState_ = D3D12_RESOURCE_STATE_COPY_DEST;
	}
	else if (type_ == TextureType::Unordered) {
		heapType_ = D3D12_HEAP_TYPE_DEFAULT;
		rscFlag_ = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		rscState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}
	auto heapProp = CD3DX12_HEAP_PROPERTIES(heapType_);
	CD3DX12_RESOURCE_DESC rscDesc;
	if (texDim_ == TextureDim::Tex1D) {
		rscDesc = CD3DX12_RESOURCE_DESC::Tex1D(format_, width_);
		rscDesc.MipLevels = 1;
	}
	else if (texDim_ == TextureDim::Tex2D)
	{
		rscDesc = CD3DX12_RESOURCE_DESC::Tex2D(format_, width_, height_);
		rscDesc.MipLevels = 1;
	}
	else if (texDim_ == TextureDim::Tex3D) {
		rscDesc = CD3DX12_RESOURCE_DESC::Tex3D(format_, width_, height_, depth_);
		rscDesc.MipLevels = 1;
	}
	rscDesc.Flags = rscFlag_;
	HRESULT result = pDevice_->GetDevice()->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &rscDesc, rscState_, nullptr, IID_PPV_ARGS(resource_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		throw std::runtime_error("Failed to create buffer : " + to_string(result));
	}
	resource_->SetName(name_.c_str());
}

Texture::Texture()
	: Resource()
{

}

void* Texture::Map()
{
	void* pData = nullptr;
	HRESULT result = resource_->Map(0, nullptr, &pData);
	if (FAILED(result)) {
		cerr << "Failed to map buffer" << endl;;
		return nullptr;
	}
	return pData;
}

void Texture::Unmap()
{
	resource_->Unmap(0, nullptr);
}

void Texture::Reset()
{
	resource_.Reset();
}

void Texture::CreateSRV(DescriptorManager& descManager, UINT viewOffset)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
	viewDesc.Format = format_;
	if (texDim_ == TextureDim::Tex1D) {
		viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
		viewDesc.Texture1D.MostDetailedMip = 0;
		viewDesc.Texture1D.MipLevels = 1;
	}
	else if (texDim_ == TextureDim::Tex2D) {
		viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MostDetailedMip = 0;
		viewDesc.Texture2D.MipLevels = 1;
		viewDesc.Texture2D.PlaneSlice = 0;
		viewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	}
	else if (texDim_ == TextureDim::Tex3D) {
		viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		viewDesc.Texture3D.MostDetailedMip = 0;
		viewDesc.Texture3D.MipLevels = 1;
		viewDesc.Texture3D.ResourceMinLODClamp = 0.0f;
	}
	viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	auto heapHandle = descManager.GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	heapHandle.ptr += pDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * viewOffset;
	pDevice_->GetDevice()->CreateShaderResourceView(resource_.Get(), &viewDesc, heapHandle);
}

void Texture::CreateUAV(DescriptorManager& descManager, UINT viewOffset)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
	viewDesc.Format = format_;
	if (texDim_ == TextureDim::Tex1D) {
		viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
		viewDesc.Texture1D.MipSlice = 0;
	}
	else if (texDim_ == TextureDim::Tex2D) {
		viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MipSlice = 0;
		viewDesc.Texture2D.PlaneSlice = 0;

	}
	else if (texDim_ == TextureDim::Tex3D) {
		viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		viewDesc.Texture3D.MipSlice = 0;
		viewDesc.Texture3D.FirstWSlice = 0;
		viewDesc.Texture3D.WSize = 1;
	}
	auto heapHandle = descManager.GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	heapHandle.ptr += pDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * viewOffset;
	pDevice_->GetDevice()->CreateUnorderedAccessView(resource_.Get(), nullptr, &viewDesc, heapHandle);
}

UINT Texture::GetStrideSize() const
{
	return strideSize_;
}

void Texture::SetName(std::wstring name)
{
	name_ = name;
}

void Texture::SetResource(ComPtr<ID3D12Resource> resource)
{
	resource_ = resource;
}

AS::AS(const Device& device, UINT size, std::wstring name)
	: Resource(device, ResourceType::AS, name), size_(size)
{
	heapType_ = D3D12_HEAP_TYPE_DEFAULT;
	rscFlag_ = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	rscState_ = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

	auto heapProp = CD3DX12_HEAP_PROPERTIES(heapType_);
	auto rscDesc = CD3DX12_RESOURCE_DESC::Buffer(size_, rscFlag_);
	HRESULT result = pDevice_->GetDevice()->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &rscDesc, rscState_, nullptr, IID_PPV_ARGS(resource_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		throw std::runtime_error("Failed to CreateCommittedResource for AS : " + to_string(result));
	}
	resource_->SetName(name_.c_str());
}

void AS::CreateSRV(DescriptorManager& descManager, UINT viewOffset)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
	viewDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	viewDesc.RaytracingAccelerationStructure.Location = resource_->GetGPUVirtualAddress();
	auto heapHandle = descManager.GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	heapHandle.ptr += pDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * viewOffset;
	pDevice_->GetDevice()->CreateShaderResourceView(nullptr, &viewDesc, heapHandle);
}