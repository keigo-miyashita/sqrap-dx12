#pragma once

#include <common.hpp>

class Device;

class Buffer
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ComPtr<ID3D12Resource> resource_ = nullptr;
	D3D12_HEAP_TYPE heapType_ = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_FLAGS rscFlag_ = D3D12_RESOURCE_FLAG_NONE;
	D3D12_RESOURCE_STATES initRscState_ = D3D12_RESOURCE_STATE_COMMON;
	UINT strideSize_;
	UINT numElement_;

	bool CreateBuffer(const Device& device, UINT strideSize, UINT numElement, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_FLAGS rscFlag, D3D12_RESOURCE_STATES initRscState, std::wstring name);

public:
	static UINT AlignForUAVCounter(UINT size);
	static UINT AlignForConstantBuffer(UINT size);

	Buffer();
	~Buffer() = default;
	bool Init(const Device& device, UINT strideSize, UINT numElement, D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAGS rscFlag = D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATES initRscState = D3D12_RESOURCE_STATE_COMMON, std::wstring name = L"Default Buffer");
	bool InitAsUpload(const Device& device,  UINT strideSize, UINT numElement, std::wstring name = L"Upload Buffer");
	bool InitAsReadback(const Device& device, UINT strideSize, UINT numElement, std::wstring name = L"Readback Buffer");
	void* Map();
	void Unmap();
	void Upload();

	ComPtr<ID3D12Resource> GetResource() const;
	D3D12_HEAP_TYPE GetHeapType() const;
	D3D12_RESOURCE_FLAGS GetResourceFlag() const;
	UINT GetStrideSize() const;
	UINT GetNumElement() const;
};