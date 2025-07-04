#pragma once

#include "pch.hpp"

#include "Alias.hpp"

class Buffer;
class Device;
class StateObject;

class WorkGraph
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const Device* pDevice_ = nullptr;
	std::wstring name_;
	ComPtr<ID3D12WorkGraphProperties1> workGraphProp_ = nullptr;
	D3D12_PROGRAM_IDENTIFIER workGraphProgramID_ = {};
	D3D12_WORK_GRAPH_MEMORY_REQUIREMENTS memReqs_ = {};
	// D3D12_GPU_VIRTUAL_ADDRESSとsizeInBytesをもつ構造体
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE backingMemoryAddressRange_ = {};
	ComPtr<ID3D12Resource> backingMemory_ = nullptr;
	BufferHandle localRootSigBuffer_;
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE localRootArgumentsTableAddress_ = {};
	UINT workGraphIndex_ = 0;
	UINT numEntryPoints_ = 0;
	UINT numNodes_ = 0;

	// CommandList.SetProgramに渡す構造体
	D3D12_SET_PROGRAM_DESC pgDesc_ = {};

public:
	static UINT AlignForLocalSigBuffer(UINT size);
	static UINT CopyMem(void* dest, const void* data, UINT size);
	WorkGraph(const Device& device, StateObjectHandle stateObject, UINT maxInputRecords = 0, UINT maxNodeInputs = 0, std::wstring name = L"");
	~WorkGraph() = default;

	D3D12_GPU_VIRTUAL_ADDRESS_RANGE GetBackingMemoryAddressRange();
	ComPtr<ID3D12Resource> GetBackingMemory();
	D3D12_PROGRAM_IDENTIFIER GetProgramID();
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetLocalSigSize();
	D3D12_SET_PROGRAM_DESC GetProgramDesc() const;
	D3D12_SET_PROGRAM_DESC* GetPProgramDesc();
};