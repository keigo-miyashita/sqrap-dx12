#pragma once

#include "pch.hpp"

#include "Alias.hpp"

class Buffer;
class Device;
class Resource;
class Texture;

enum class HeapType
{
	Resource, Sampler,
};

enum class ViewType
{
	NONE, CBV, SRV, UAV, SAMPLER,
};

struct DescriptorManagerDesc
{
	ResourceHandle resource_;
	ViewType type_;
	UINT numReg_;
	bool isCounter_ = false;
};

class DescriptorManager
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const Device* pDevice_ = nullptr;
	// CBV_SRV_UABまたはSamplerを1つずつ（キューに）セットできる
	HeapType heapType_;
	std::wstring name_;
	ComPtr<ID3D12DescriptorHeap> descHeap_ = nullptr;
	UINT viewOffset_ = 0;
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

	void CreateCBV(ResourceHandle resource);
	void CreateSRV(ResourceHandle resource);
	void CreateUAV(ResourceHandle resource);
	void CreateUAVCounter(ResourceHandle resource);
	void CreateSampler();

public:
	DescriptorManager(const Device& pDevice, HeapType heapType, std::initializer_list<DescriptorManagerDesc> descManagerDesc, std::wstring name = L"");
	~DescriptorManager() = default;
	ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() const;
	HeapType GetHeapType() const;
	UINT GetNumDescRanges() const;
	const CD3DX12_DESCRIPTOR_RANGE* GetPDescRanges() const;
};