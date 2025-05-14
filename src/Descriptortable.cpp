#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

void DescriptorTable::CreateBufferDescriptorRanges(UINT baseRegCBV, UINT numCBV, UINT baseRegSRV, UINT numSRV, UINT baseRegUAV, UINT numUAV)
{
	numCBV_ = numCBV;
	numSRV_ = numSRV;
	numUAV_ = numUAV;
	if (numCBV != 0) {
		CD3DX12_DESCRIPTOR_RANGE descRange = {};
		descRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, numCBV, baseRegCBV, 0, 0);
		descRanges_.push_back(descRange);
	}
	if (numSRV != 0) {
		CD3DX12_DESCRIPTOR_RANGE descRange = {};
		descRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, numSRV, baseRegSRV, 0, numCBV);
		descRanges_.push_back(descRange);
	}
	if (numUAV != 0) {
		CD3DX12_DESCRIPTOR_RANGE descRange = {};
		descRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, numUAV, baseRegUAV, 0, numCBV + numSRV);
		descRanges_.push_back(descRange);
	}
}

void DescriptorTable::CreateSamplerDescriptorRanges(UINT baseRegSampler, UINT numSampler)
{
	numSampler_ = numSampler;
	baseRegSampler_ = baseRegSampler;
	if (numSampler != 0) {
		CD3DX12_DESCRIPTOR_RANGE descRange = {};
		descRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, numSampler, baseRegSampler, 0, 0);
		descRanges_.push_back(descRange);
	}
}

DescriptorTable::DescriptorTable()
{

}

void DescriptorTable::InitAsBuffer(UINT baseRegCBV, UINT numCBV, UINT baseRegSRV, UINT numSRV, UINT baseRegUAV, UINT numUAV)
{
	heapType_ = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	CreateBufferDescriptorRanges(baseRegCBV, numCBV, baseRegSRV, numSRV, baseRegUAV, numUAV);
}

void DescriptorTable::InitAsSampler(UINT baseRegSampler, UINT numSampler)
{
	heapType_ = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	CreateSamplerDescriptorRanges(baseRegSampler, numSampler);
}

UINT DescriptorTable::GetNumDescRanges() const
{
	return descRanges_.size();
}

const CD3DX12_DESCRIPTOR_RANGE* DescriptorTable::GetPDescRanges() const
{
	return descRanges_.data();
}