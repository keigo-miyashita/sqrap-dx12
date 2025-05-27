#pragma once

#include <common.hpp>

class Device;

enum class HeapType
{
	Buffer,
	Sampler,
};

enum class ViewType
{
	NONE,
	CBV,
	SRV,
	UAV,
	SAMPLER,
};

struct DescriptorManagerDesc
{
	const Buffer& buffer;
	ViewType type;
	UINT numReg;
	bool isCounter = false;
};

class DescriptorManager
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const Device* pDevice_ = nullptr;
	// CBV_SRV_UAB�܂���Sampler��1���i�L���[�Ɂj�Z�b�g�ł���
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

	void CreateCBV(const Buffer& buff);
	void CreateSRV(const Buffer& buff);
	void CreateUAV(const Buffer& buff);
	void CreateUAVCounter(const Buffer& buff);
	void CreateSampler();

public:
	DescriptorManager(const Device& pDevice, HeapType heapType, std::initializer_list<DescriptorManagerDesc> descManagerDesc, std::wstring name = L"");
	~DescriptorManager() = default;
	ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() const;
	HeapType GetHeapType() const;
	UINT GetNumDescRanges() const;
	const CD3DX12_DESCRIPTOR_RANGE* GetPDescRanges() const;
};