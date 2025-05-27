#pragma once

#include <common.hpp>

enum class BufferType
{
	Default, Upload, Read, Unordered, AS, Counter
};

class Device;

class Buffer
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const Device* pDevice_ = nullptr;
	BufferType type_;
	std::wstring name_;
	ComPtr<ID3D12Resource> resource_ = nullptr;
	D3D12_HEAP_TYPE heapType_ = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_FLAGS rscFlag_ = D3D12_RESOURCE_FLAG_NONE;
	D3D12_RESOURCE_STATES rscState_ = D3D12_RESOURCE_STATE_COMMON;
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
	/*bool Init();
	bool InitAsCounter();
	bool InitAsUpload();
	bool InitAsReadback();*/
	void* Map();
	void Unmap();
	void Reset();

	ComPtr<ID3D12Resource> GetResource() const;
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const;
	D3D12_HEAP_TYPE GetHeapType() const;
	D3D12_RESOURCE_FLAGS GetResourceFlag() const;
	D3D12_RESOURCE_STATES GetResourceState() const;
	UINT GetStrideSize() const;
	UINT GetNumElement() const;
	UINT GetOffsetCounter() const;

	void SetResourceState(D3D12_RESOURCE_STATES rscState);
};