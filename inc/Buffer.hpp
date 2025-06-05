#pragma once

#include <common.hpp>

enum class ResourceType
{
	Buffer, Texture
};

enum class BufferType
{
	Default, Upload, Read, Unordered, AS, Counter
};

class DescriptorManager;
class Device;

class Resource
{
protected:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const Device* pDevice_ = nullptr;
	ComPtr<ID3D12Resource> resource_ = nullptr;
	std::wstring name_;
	ResourceType rscType_;;
	D3D12_HEAP_TYPE heapType_ = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_FLAGS rscFlag_ = D3D12_RESOURCE_FLAG_NONE;
	D3D12_RESOURCE_STATES rscState_ = D3D12_RESOURCE_STATE_COMMON;
	UINT strideSize_ = 0;

public:
	Resource(const Device& device, ResourceType rscType, UINT strideSize, std::wstring name = L"");
	~Resource() = default;
	
	virtual void CreateCBV(DescriptorManager& descManager, UINT viewOffset);
	virtual void CreateSRV(DescriptorManager& descManager, UINT viewOffset) = 0;
	virtual void CreateUAV(DescriptorManager& descManager, UINT viewOffset) = 0;
	virtual void CreateUAVCounter(DescriptorManager& descManager, UINT viewOffset);

	ComPtr<ID3D12Resource> GetResource() const;
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const;
	ResourceType GetResourceType() const;
	D3D12_HEAP_TYPE GetHeapType() const;
	D3D12_RESOURCE_FLAGS GetResourceFlag() const;
	D3D12_RESOURCE_STATES GetResourceState() const;
	UINT GetStrideSize() const;

	void SetResourceState(D3D12_RESOURCE_STATES rscState);
};

class Buffer : public Resource
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	BufferType type_;
	UINT numElement_ = 0;
	UINT offsetCounter_ = 0;

	bool CreateBuffer();
	bool CreateCounterBuffer();

public:
	static UINT AlignForUAVCounter(UINT size);
	static UINT AlignForConstantBuffer(UINT size);

	Buffer(const Device& device, BufferType type, UINT strideSize, UINT numElement, std::wstring name = L"");
	~Buffer() = default;

	void* Map();
	void Unmap();
	void Reset();

	void CreateCBV(DescriptorManager& descManager, UINT viewOffset) override;
	void CreateSRV(DescriptorManager& descManager, UINT viewOffset) override;
	void CreateUAV(DescriptorManager& descManager, UINT viewOffset) override;
	void CreateUAVCounter(DescriptorManager& descManager, UINT viewOffset) override;

	UINT GetNumElement() const;
	UINT GetOffsetCounter() const;;
};

enum class TextureDimention
{
	Tex1D, Tex2D, Tex3D,
};

enum class TextureType
{
	Default, Upload, Read, Unordered
};

class Texture : public Resource
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	TextureType type_;
	TextureDimention texDim_;
	UINT width_ = 1;
	UINT height_ = 1;
	UINT depth_ = 1;
	DXGI_FORMAT format_;

public:
	Texture(const Device& device, TextureDimention texDim, TextureType type, UINT strideSize, DXGI_FORMAT format, UINT width, UINT height, UINT depth, std::wstring name = L"");
	~Texture() = default;
	void* Map();
	void Unmap();
	void Reset();

	void CreateSRV(DescriptorManager& descManager, UINT viewOffset) override;
	void CreateUAV(DescriptorManager& descManager, UINT viewOffset) override;

	UINT GetWidth() const;
	UINT GetHeight() const;
	UINT GetDepth() const;
};