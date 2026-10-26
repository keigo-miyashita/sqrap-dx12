#pragma once

#include "pch.hpp"

#include "RootSignature.hpp"

#include "Alias.hpp"

namespace sqrp
{
	class Buffer;
	class Device;
	class StateObject;

	class WorkGraph
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;
		using IndexInWorkGraphDesc = UINT;

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

	public:
		static UINT AlignForLocalSigBuffer(UINT size);
		static UINT CopyMem(void* dest, const void* data, UINT size);
		D3D12_SET_PROGRAM_DESC MakeSetProgramDesc(bool initialize = true)const;
		WorkGraph(const Device& device, std::wstring name, const StateObjectHandle& stateObject,
			std::vector<std::vector<LocalRootArguments>> computeLocalRootArguments,
			std::vector<std::vector<LocalRootArguments>> graphicsLocalRootArguments,
			UINT maxInputRecords = 0, UINT maxNodeInputs = 0);
		WorkGraph(const WorkGraph&) = delete;
		WorkGraph& operator=(const WorkGraph&) = delete;

		D3D12_GPU_VIRTUAL_ADDRESS_RANGE GetBackingMemoryAddressRange() const;
		const ComPtr<ID3D12Resource>& GetBackingMemory() const;
		D3D12_PROGRAM_IDENTIFIER GetProgramID() const;
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetLocalRootArgumentsTable() const;
	};
}
