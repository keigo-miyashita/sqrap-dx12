#include "Resource.hpp"

#include "DescriptorManager.hpp"
#include "Device.hpp"

#include <stb_image.h>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	Resource::Resource(const Device& device, ResourceType rscType, wstring name)
		: device_(device), name_(name), rscType_(rscType)
	{

	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC Resource::MakeCBV() const
	{
		throw runtime_error("[Resource::MakeCBV] a CBV cannot be created for this resource type");
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC Resource::MakeSRV() const
	{
		throw runtime_error("[Resource::MakeSRV] an SRV cannot be created for this resource type");
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC Resource::MakeUAV(bool isCounter, UINT mipSlice) const
	{
		throw runtime_error("[Resource::MakeUAV] a UAV cannot be created for this resource type");
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

	D3D12_RESOURCE_STATES Resource::GetInitialState() const
	{
		return initialState_;
	}

	const ComPtr<ID3D12Resource>& Resource::GetResource() const
	{
		return resource_;
	}

	D3D12_GPU_VIRTUAL_ADDRESS Resource::GetGPUAddress() const
	{
		return resource_->GetGPUVirtualAddress();
	}

	/*void Resource::SetResourceState(D3D12_RESOURCE_STATES rscState)
	{
		initialState_ = rscState;
	}*/

	void Buffer::CreateBuffer()
	{
		auto heapProp = CD3DX12_HEAP_PROPERTIES(heapType_);
		auto rscDesc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(strideSize_) * numElement_, rscFlag_);
		HRESULT result = device_.GetDevice()->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &rscDesc, initialState_, nullptr, IID_PPV_ARGS(resource_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[Buffer::CreateBuffer] CreateCommittedResource failed\nHRESULT = " + to_string(result));
		}
		resource_->SetName((name_ + L"_Buffer").c_str());
	}

	void Buffer::CreateCounterBuffer()
	{
		heapType_ = D3D12_HEAP_TYPE_DEFAULT;
		rscFlag_ = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		initialState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		auto heapProp = CD3DX12_HEAP_PROPERTIES(heapType_);
		const UINT64 dataSize = static_cast<UINT64>(strideSize_) * numElement_;
		offsetCounter_ = AlignForUAVCounter(dataSize);
		auto rscDesc = CD3DX12_RESOURCE_DESC::Buffer(offsetCounter_ + sizeof(UINT), rscFlag_);
		HRESULT result = device_.GetDevice()->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &rscDesc, initialState_, nullptr, IID_PPV_ARGS(resource_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[Buffer::CreateCounterBuffer] CreateCommittedResource failed\nHRESULT = " + to_string(result));
		}
		resource_->SetName((name_ + L"_Counter_Buffer").c_str());
	}

	UINT64 Buffer::AlignForUAVCounter(UINT64 size)
	{
		const UINT64 alignment = D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT;
		return (size + (alignment - 1)) & ~(alignment - 1);
	}

	UINT Buffer::AlignForConstantBuffer(UINT size)
	{
		return (size + 0xff) & ~0xff;
	}

	Buffer::Buffer(const Device& device, wstring name, BufferType type, UINT strideSize, UINT numElement, D3D12_RESOURCE_FLAGS resourceFlag)
		: Resource(device, ResourceType::Buffer, name), type_(type), strideSize_(strideSize), numElement_(numElement)
	{
		// 呼び出し側の契約違反。CreateCommittedResource に渡す前に自分で検出する
		// (サイズ0のまま渡すと E_INVALIDARG になり、環境エラーと区別がつかなくなる)
		if (strideSize == 0 || numElement == 0) {
			throw runtime_error("[Buffer::Buffer] strideSize and numElement must be greater than 0");
		}
		if (type == BufferType::Counter) {
			heapType_ = D3D12_HEAP_TYPE_DEFAULT;
			rscFlag_ = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | resourceFlag;
			initialState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			CreateCounterBuffer();
		}
		else {
			if (type == BufferType::Default) {
				heapType_ = D3D12_HEAP_TYPE_DEFAULT;
				rscFlag_ = D3D12_RESOURCE_FLAG_NONE | resourceFlag;
				initialState_ = D3D12_RESOURCE_STATE_COMMON;
			}
			else if (type == BufferType::Upload) {
				heapType_ = D3D12_HEAP_TYPE_UPLOAD;
				rscFlag_ = D3D12_RESOURCE_FLAG_NONE | resourceFlag;
				initialState_ = D3D12_RESOURCE_STATE_COPY_SOURCE;
			}
			else if (type == BufferType::Read) {
				heapType_ = D3D12_HEAP_TYPE_READBACK;
				rscFlag_ = D3D12_RESOURCE_FLAG_NONE | resourceFlag;
				initialState_ = D3D12_RESOURCE_STATE_COPY_DEST;
			}
			else if (type == BufferType::Unordered) {
				heapType_ = D3D12_HEAP_TYPE_DEFAULT;
				rscFlag_ = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | resourceFlag;
				initialState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			}
			else if (type == BufferType::AS) {
				heapType_ = D3D12_HEAP_TYPE_DEFAULT;
				rscFlag_ = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | resourceFlag;
				initialState_ = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
			}
			CreateBuffer();
		}
	}

	void* Buffer::Map(UINT subresource, D3D12_RANGE* range)
	{
		void* pData = nullptr;
		HRESULT result = resource_->Map(subresource, range, &pData);
		if (FAILED(result)) {
			cerr << "[Buffer::Map] Map failed" << endl;
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

	void Buffer::Upload(void* pData, UINT strideSize, UINT numElement)
	{
		void* rawPtr = this->Map();
		if (rawPtr) {
			memcpy(rawPtr, pData, static_cast<size_t>(strideSize) * numElement);
			this->Unmap();
		}
		else {
			cerr << "[Buffer::Upload] upload failed because Map failed" << endl;
		}
	}

	void Buffer::Write(const void* src, UINT size)
	{
		if (size > static_cast<std::size_t>(GetStrideSize()) * GetNumElement()) {
			throw std::runtime_error("[Buffer::Write] the size of src is larger than the buffer size");
		}
		void* rawPtr = Map();
		if (!rawPtr) {
			throw std::runtime_error("[Buffer::Write] Map failed");
		}
		memcpy(rawPtr, src, size);
		Unmap();
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC Buffer::MakeCBV() const
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = {};
		viewDesc.BufferLocation = resource_->GetGPUVirtualAddress();
		viewDesc.SizeInBytes = strideSize_ * numElement_;
		return viewDesc;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC Buffer::MakeSRV() const
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
		viewDesc.Format = DXGI_FORMAT_UNKNOWN;
		viewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		viewDesc.Buffer.StructureByteStride = strideSize_;
		viewDesc.Buffer.NumElements = numElement_;
		viewDesc.Buffer.FirstElement = 0;
		viewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		return viewDesc;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC Buffer::MakeUAV(bool isCounter, UINT mipSlice) const
	{
		if (mipSlice > 0) {
			throw runtime_error("[Buffer::MakeUAV] mipSlice must be 0 for a buffer UAV");
		}
		D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
		viewDesc.Format = DXGI_FORMAT_UNKNOWN;
		viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		viewDesc.Buffer.StructureByteStride = strideSize_;
		viewDesc.Buffer.NumElements = numElement_;
		viewDesc.Buffer.FirstElement = 0;
		viewDesc.Buffer.CounterOffsetInBytes = isCounter ? offsetCounter_ : 0;
		viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		return viewDesc;
	}

	UINT Buffer::GetStrideSize() const
	{
		return strideSize_;
	}

	UINT Buffer::GetNumElement() const
	{
		return numElement_;
	}

	UINT64 Buffer::GetOffsetCounter() const
	{
		return offsetCounter_;
	}

	Texture::Texture(const Device& device, wstring name, TextureDim texDim, TextureType type, UINT strideSize, DXGI_FORMAT format, UINT width, UINT height, UINT depth, UINT mipLevels)
		: Resource(device, ResourceType::Texture, name), type_(type), texDim_(texDim), strideSize_(strideSize), width_(width), height_(height), depth_(depth), mipLevels_(mipLevels), format_(format)
	{
		if (type_ == TextureType::Default) {
			heapType_ = D3D12_HEAP_TYPE_DEFAULT;
			rscFlag_ = D3D12_RESOURCE_FLAG_NONE;
			initialState_ = D3D12_RESOURCE_STATE_COMMON;
		}
		else if (type_ == TextureType::Upload) {
			heapType_ = D3D12_HEAP_TYPE_UPLOAD;
			rscFlag_ = D3D12_RESOURCE_FLAG_NONE;
			initialState_ = D3D12_RESOURCE_STATE_COPY_SOURCE;
		}
		else if (type_ == TextureType::Read) {
			heapType_ = D3D12_HEAP_TYPE_READBACK;
			rscFlag_ = D3D12_RESOURCE_FLAG_NONE;
			initialState_ = D3D12_RESOURCE_STATE_COPY_DEST;
		}
		else if (type_ == TextureType::Unordered) {
			heapType_ = D3D12_HEAP_TYPE_DEFAULT;
			rscFlag_ = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			initialState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}
		auto heapProp = CD3DX12_HEAP_PROPERTIES(heapType_);
		CD3DX12_RESOURCE_DESC rscDesc;
		if (texDim_ == TextureDim::Tex1D) {
			rscDesc = CD3DX12_RESOURCE_DESC::Tex1D(format_, width_);
			rscDesc.MipLevels = mipLevels_;
		}
		else if (texDim_ == TextureDim::Tex2D)
		{
			rscDesc = CD3DX12_RESOURCE_DESC::Tex2D(format_, width_, height_);
			rscDesc.MipLevels = mipLevels_;
		}
		else if (texDim_ == TextureDim::Tex3D) {
			rscDesc = CD3DX12_RESOURCE_DESC::Tex3D(format_, width_, height_, depth_);
			rscDesc.MipLevels = mipLevels_;
		}
		else if (texDim_ == TextureDim::TexCube) {
			rscDesc = CD3DX12_RESOURCE_DESC::Tex2D(format_, width_, height_, 6);
			rscDesc.MipLevels = mipLevels_;
		}
		rscDesc.Flags = rscFlag_;
		HRESULT result = device_.GetDevice()->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &rscDesc, initialState_, nullptr, IID_PPV_ARGS(resource_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[Texture::Texture] CreateCommittedResource failed\nHRESULT = " + to_string(result));
		}
		resource_->SetName((name_ + L"_Texture").c_str());
	}

	Texture::Texture(const Device& device, wstring name, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES initialState)
		: Resource(device, ResourceType::Texture, name)
	{
		if (!resource) {
			throw runtime_error("[Texture::Texture] resource must not be nullptr");
		}
		resource_ = resource;
		initialState_ = initialState;

		const D3D12_RESOURCE_DESC desc = resource_->GetDesc();
		if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D) {
			texDim_ = TextureDim::Tex1D;
		}
		else if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
			texDim_ = TextureDim::Tex2D;
		}
		else if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
			texDim_ = TextureDim::Tex3D;
		}
		else {
			throw runtime_error("[Texture::Texture] resource is not a texture");
		}
		width_ = static_cast<UINT>(desc.Width);
		height_ = desc.Height;
		depth_ = (texDim_ == TextureDim::Tex3D) ? desc.DepthOrArraySize : 1;
		mipLevels_ = desc.MipLevels;
		format_ = desc.Format;
		strideSize_ = static_cast<UINT>(BitsPerPixel(format_) / 8);
		rscFlag_ = desc.Flags;
		type_ = (rscFlag_ & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) ? TextureType::Unordered : TextureType::Default;

		D3D12_HEAP_PROPERTIES heapProp = {};
		if (SUCCEEDED(resource_->GetHeapProperties(&heapProp, nullptr))) {
			heapType_ = heapProp.Type;
		}

		resource_->SetName((name_ + L"_Texture").c_str());
	}

	void* Texture::Map()
	{
		void* pData = nullptr;
		HRESULT result = resource_->Map(0, nullptr, &pData);
		if (FAILED(result)) {
			cerr << "[Texture::Map] Map failed" << endl;
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

	D3D12_SHADER_RESOURCE_VIEW_DESC Texture::MakeSRV() const
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
		viewDesc.Format = format_;
		if (texDim_ == TextureDim::Tex1D) {
			viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
			viewDesc.Texture1D.MostDetailedMip = 0;
			viewDesc.Texture1D.MipLevels = mipLevels_;
		}
		else if (texDim_ == TextureDim::Tex2D) {
			viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			viewDesc.Texture2D.MostDetailedMip = 0;
			viewDesc.Texture2D.MipLevels = mipLevels_;
			viewDesc.Texture2D.PlaneSlice = 0;
			viewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		}
		else if (texDim_ == TextureDim::Tex3D) {
			viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			viewDesc.Texture3D.MostDetailedMip = 0;
			viewDesc.Texture3D.MipLevels = mipLevels_;
			viewDesc.Texture3D.ResourceMinLODClamp = 0.0f;
		}
		else if (texDim_ == TextureDim::TexCube) {
			viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			viewDesc.TextureCube.MostDetailedMip = 0;
			viewDesc.TextureCube.MipLevels = mipLevels_;
			viewDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		}
		viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		return viewDesc;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC Texture::MakeUAV(bool isCounter, UINT mipSlice) const
	{
		if (isCounter) {
			throw runtime_error("[Texture::MakeUAV] a texture UAV cannot have a counter");
		}
		D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
		viewDesc.Format = format_;
		if (texDim_ == TextureDim::Tex1D) {
			viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
			viewDesc.Texture1D.MipSlice = mipSlice;
		}
		else if (texDim_ == TextureDim::Tex2D) {
			viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			viewDesc.Texture2D.MipSlice = mipSlice;
			viewDesc.Texture2D.PlaneSlice = 0;

		}
		else if (texDim_ == TextureDim::Tex3D) {
			viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			viewDesc.Texture3D.MipSlice = mipSlice;
			viewDesc.Texture3D.FirstWSlice = 0;
			viewDesc.Texture3D.WSize = 1;
		}
		else if (texDim_ == TextureDim::TexCube) {
			viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			viewDesc.Texture2DArray.MipSlice = mipSlice;
			viewDesc.Texture2DArray.FirstArraySlice = 0;
			viewDesc.Texture2DArray.ArraySize = 6;
			viewDesc.Texture2DArray.PlaneSlice = 0;
		}
		return viewDesc;
	}

	UINT Texture::GetStrideSize() const
	{
		return strideSize_;
	}

	UINT Texture::GetWidth() const
	{
		return width_;
	}

	UINT Texture::GetHeight() const
	{
		return height_;
	}

	UINT Texture::GetDepth() const
	{
		return depth_;
	}

	UINT Texture::GetMipLevels() const
	{
		return mipLevels_;
	}

	AS::AS(const Device& device, wstring name, UINT64 size)
		: Resource(device, ResourceType::AS, name), size_(size)
	{
		heapType_ = D3D12_HEAP_TYPE_DEFAULT;
		rscFlag_ = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		initialState_ = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

		auto heapProp = CD3DX12_HEAP_PROPERTIES(heapType_);
		auto rscDesc = CD3DX12_RESOURCE_DESC::Buffer(size_, rscFlag_);
		HRESULT result = device_.GetDevice()->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &rscDesc, initialState_, nullptr, IID_PPV_ARGS(resource_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[AS::AS] CreateCommittedResource failed\nHRESULT = " + to_string(result));
		}
		resource_->SetName((name_ + L"_AS").c_str());
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC AS::MakeSRV() const
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
		viewDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
		viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		viewDesc.RaytracingAccelerationStructure.Location = resource_->GetGPUVirtualAddress();
		return viewDesc;
	}

	Constants::Constants(void* constants, UINT numConstants)
		: constants_(constants), numConstants_(numConstants)
	{

	}

	void* Constants::GetConstants() const
	{
		return constants_;
	}

	UINT Constants::GetNumConstants() const
	{
		return numConstants_;
	}

	HDRImage LoadHDR(const string& path)
	{
		HDRImage img;
		unique_ptr<float, decltype(&stbi_image_free)> raw(stbi_loadf(path.c_str(), &img.width, &img.height, &img.channels, 4), &stbi_image_free);
		if (!raw) {
			throw runtime_error("[LoadHDR] stbi_loadf failed for " + path);
		}
		img.channels = 4;
		const size_t dataSize = static_cast<size_t>(img.width) * img.height * img.channels;
		img.data.assign(raw.get(), raw.get() + dataSize);
		return img;
	}
}
