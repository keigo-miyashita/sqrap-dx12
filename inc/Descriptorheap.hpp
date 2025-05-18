#pragma once

#include <common.hpp>

class Device;

class DescriptorHeap
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	Device* pDevice_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> descHeap_ = nullptr;
	// CBV_SRV_UABまたはSamplerを1つずつ（キューに）セットできる
	D3D12_DESCRIPTOR_HEAP_TYPE heapType_;

	bool CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptor, std::wstring name = L"DescriptorHeap");

public:
	DescriptorHeap();
	~DescriptorHeap() = default;
	bool Init(Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptor, std::wstring name = L"DescriptorHeap");
	void CreateCBV(const Buffer& buff, UINT viewOffset);
	void CreateSRV(const Buffer& buff, UINT viewOffset);
	void CreateUAV(const Buffer& buff, UINT viewOffset);
	void CreateUAVCounter(const Buffer& buff, UINT viewOffset);
	void CreateSampler(UINT viewOffset);
	ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap();
	D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType();
};