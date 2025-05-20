#pragma once

#include <common.hpp>

class Device;

class Buffer
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	Device* pDevice_ = nullptr;
	ComPtr<ID3D12Resource> resource_ = nullptr;
	D3D12_HEAP_TYPE heapType_ = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_FLAGS rscFlag_ = D3D12_RESOURCE_FLAG_NONE;
	D3D12_RESOURCE_STATES rscState_ = D3D12_RESOURCE_STATE_COMMON;
	UINT strideSize_;
	UINT numElement_;
	UINT offsetCounter_;

	bool CreateBuffer(UINT strideSize, UINT numElement, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_FLAGS rscFlag, D3D12_RESOURCE_STATES initRscState, std::wstring name);
	bool CreateCounterBuffer(UINT strideSize, UINT numElement, std::wstring name);

public:
	static UINT AlignForUAVCounter(UINT size);
	static UINT AlignForConstantBuffer(UINT size);

	Buffer();
	~Buffer() = default;
	bool Init(Device* pDevice, UINT strideSize, UINT numElement, D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAGS rscFlag = D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATES initRscState = D3D12_RESOURCE_STATE_COMMON, std::wstring name = L"Default Buffer");
	bool InitAsCounter(Device* pDevice, UINT strideSize, UINT numElement, std::wstring name = L"Counter Buffer");
	bool InitAsUpload(Device* pDevice,  UINT strideSize, UINT numElement, std::wstring name = L"Upload Buffer");
	bool InitAsReadback(Device* pDevice, UINT strideSize, UINT numElement, std::wstring name = L"Readback Buffer");
	void* Map();
	void Unmap();
	void Reset();

	ComPtr<ID3D12Resource> GetResource() const;
	D3D12_HEAP_TYPE GetHeapType() const;
	D3D12_RESOURCE_FLAGS GetResourceFlag() const;
	D3D12_RESOURCE_STATES GetResourceState() const;
	UINT GetStrideSize() const;
	UINT GetNumElement() const;
	UINT GetOffsetCounter() const;

	void SetResourceState(D3D12_RESOURCE_STATES rscState);
};