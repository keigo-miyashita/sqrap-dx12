#pragma once

#include <common.hpp>

class Buffer;
class Device;

class WorkGraph
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	Device* pDevice_ = nullptr;
	ComPtr<ID3D12WorkGraphProperties1> workGraphProp_ = nullptr;
	D3D12_PROGRAM_IDENTIFIER workGraphProgramID_ = {};
	D3D12_WORK_GRAPH_MEMORY_REQUIREMENTS memReqs_ = {};
	// D3D12_GPU_VIRTUAL_ADDRESSÇ∆sizeInBytesÇÇ‡Ç¬ç\ë¢ëÃ
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE backingMemoryAddressRange_ = {};
	ComPtr<ID3D12Resource> backingMemory_ = nullptr;
	UINT workGraphIndex_ = 0;
	UINT numEntryPoints_ = 0;
	UINT numNodes_ = 0;

	// CommandList.SetProgramÇ…ìnÇ∑ç\ë¢ëÃ
	D3D12_SET_PROGRAM_DESC pgDesc_ = {};

	bool InitWorkGraphContext(const StateObject& stateObject);

public:
	WorkGraph();
	~WorkGraph() = default;
	bool Init(Device* pDevice, const StateObject& stateObject);

	D3D12_SET_PROGRAM_DESC GetProgramDesc() const;
};