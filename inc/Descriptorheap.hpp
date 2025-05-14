#pragma once

#include <common.hpp>

class Device;

class DescriptorHeap
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ComPtr<ID3D12DescriptorHeap> descHeap_ = nullptr;
	// CBV_SRV_UABまたはSamplerを1つずつ（キューに）セットできる
	D3D12_DESCRIPTOR_HEAP_TYPE heapType_;

	bool CreateDescriptorHeap(const Device& device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptor, std::wstring name = L"DescriptorHeap");

public:
	DescriptorHeap();
	~DescriptorHeap() = default;
	bool Init(const Device& device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptor, std::wstring name = L"DescriptorHeap");
	void CreateCBV(const Device& device, const Buffer& buff, UINT viewOffset);
	void CreateSRV(const Device& device, const Buffer& buff, UINT viewOffset);
	void CreateUAV(const Device& device, const Buffer& buff, UINT viewOffset);
	void CreateUAVCounter(const Device& device, const Buffer& buff, UINT viewOffset);
	void CreateSampler(const Device& device, UINT viewOffset);
	ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap();
	D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType();
};