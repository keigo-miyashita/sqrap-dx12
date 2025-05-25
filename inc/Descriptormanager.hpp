#pragma once

#include <common.hpp>

class Device;

class DescriptorManager
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	Device* pDevice_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> descHeap_ = nullptr;
	UINT viewOffset_ = 0;
	// CBV_SRV_UABまたはSamplerを1つずつ（キューに）セットできる
	D3D12_DESCRIPTOR_HEAP_TYPE heapType_;
	UINT numDescriptor_ = 0;
	// Descriptor Table
	std::vector<CD3DX12_DESCRIPTOR_RANGE> descRanges_;
	UINT baseRegCBV_ = 0;
	UINT numCBV_ = 0;
	UINT baseRegSRV_ = 0;
	UINT numSRV_ = 0;
	UINT baseRegUAV_ = 0;
	UINT numUAV_ = 0;
	UINT baseRegSampler_ = 0;
	UINT numSampler_ = 0;

	bool CreateDescriptorHeap(std::wstring name = L"DescriptorHeap");
	bool CreateDescriptorTable();

public:
	DescriptorManager();
	~DescriptorManager() = default;
	bool InitAsBuffer(Device* pDevice, UINT baseRegCBV, UINT numCBV, UINT baseRegSRV, UINT numSRV, UINT baseRegUAV, UINT numUAV, std::wstring name = L"DescriptorHeap");
	bool InitAsSampler(Device* pDevice, UINT baseRegSampler, UINT numSampler, std::wstring name = L"DescriptorHeap");
	void CreateCBV(const Buffer& buff);
	void CreateSRV(const Buffer& buff);
	void CreateUAV(const Buffer& buff);
	void CreateUAVCounter(const Buffer& buff);
	void CreateSampler();
	ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() const;
	D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const;
	UINT GetNumDescRanges() const;
	const CD3DX12_DESCRIPTOR_RANGE* GetPDescRanges() const;
};