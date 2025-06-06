#pragma once

#include "pch.hpp"

class DescriptorManager;
class Device;

enum class ResourceType
{
	Buffer, Texture, AS
};

class Resource
{
protected:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const Device* pDevice_ = nullptr;
	ComPtr<ID3D12Resource> resource_ = nullptr;
	std::wstring name_;
	ResourceType rscType_;
	D3D12_HEAP_TYPE heapType_ = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_FLAGS rscFlag_ = D3D12_RESOURCE_FLAG_NONE;
	D3D12_RESOURCE_STATES rscState_ = D3D12_RESOURCE_STATE_COMMON;

public:
	Resource(const Device& device, ResourceType rscType, std::wstring name = L"");
	Resource();
	~Resource() = default;
	
	virtual void CreateCBV(DescriptorManager& descManager, UINT viewOffset);
	virtual void CreateSRV(DescriptorManager& descManager, UINT viewOffset) = 0;
	virtual void CreateUAV(DescriptorManager& descManager, UINT viewOffset);
	virtual void CreateUAVCounter(DescriptorManager& descManager, UINT viewOffset);

	ComPtr<ID3D12Resource> GetResource() const;
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const;
	ResourceType GetResourceType() const;
	D3D12_HEAP_TYPE GetHeapType() const;
	D3D12_RESOURCE_FLAGS GetResourceFlag() const;
	D3D12_RESOURCE_STATES GetResourceState() const;

	void SetResourceState(D3D12_RESOURCE_STATES rscState);
};

enum class BufferType
{
	Default, Upload, Read, Unordered, AS, Counter
};

class Buffer : public Resource
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	BufferType type_;
	UINT strideSize_ = 0;
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

	UINT GetStrideSize() const;
	UINT GetNumElement() const;
	UINT GetOffsetCounter() const;;
};

enum class TextureType
{
	Default, Upload, Read, Unordered
};

enum class TextureDim
{
	Tex1D, Tex2D, Tex3D,
};

class Texture : public Resource
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	TextureType type_;
	TextureDim texDim_;
	UINT strideSize_ = 0;
	UINT width_ = 1;
	UINT height_ = 1;
	UINT depth_ = 1;
	DXGI_FORMAT format_;

public:
	Texture(const Device& device, TextureDim texDim, TextureType type, UINT strideSize, DXGI_FORMAT format, UINT width, UINT height, UINT depth, std::wstring name = L"");
	Texture();
	~Texture() = default;
	void* Map();
	void Unmap();
	void Reset();

	void CreateSRV(DescriptorManager& descManager, UINT viewOffset) override;
	void CreateUAV(DescriptorManager& descManager, UINT viewOffset) override;

	UINT GetStrideSize() const;
	UINT GetWidth() const;
	UINT GetHeight() const;
	UINT GetDepth() const;

	void SetName(std::wstring name);
	void SetResource(ComPtr<ID3D12Resource> resource);
};

class AS : public Resource
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	UINT size_;

public:
	AS(const Device& device, UINT size, std::wstring name = L"");
	~AS() = default;

	void CreateSRV(DescriptorManager& descManager, UINT viewOffset) override;
};