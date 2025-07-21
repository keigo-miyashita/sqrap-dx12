#include "Workgraph.hpp"

#include "Device.hpp"
#include "Resource.hpp"
#include "Pipeline.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

UINT WorkGraph::AlignForLocalSigBuffer(UINT size)
{
	const UINT alignment = 8; // sizeof(D3D12_GPU_VIRTUAL_ADDRESS, D3D12_DESC_HANDLE)
	return (size + (alignment - 1)) & ~(alignment - 1);
}

UINT WorkGraph::CopyMem(void* dest, const void* data, UINT size)
{
	memcpy(dest, data, size);

	return size;
}

WorkGraph::WorkGraph(const Device& device, StateObjectHandle stateObject, UINT maxInputRecords, UINT maxNodeInputs, std::wstring name)
	: pDevice_(&device), name_(name)
{
	ComPtr<ID3D12StateObjectProperties1> soProp = nullptr;
	auto result = stateObject->GetStateObject()->QueryInterface(IID_PPV_ARGS(soProp.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		throw std::runtime_error("Failed to QueryInterface for StateObjectProperties1 : " + to_string(result));
	}
	
	wcout << stateObject->GetProgramName().c_str() << endl;
	workGraphProgramID_ = soProp->GetProgramIdentifier(stateObject->GetProgramName().c_str());

	stateObject->GetStateObject()->QueryInterface(IID_PPV_ARGS(workGraphProp_.ReleaseAndGetAddressOf()));
	workGraphIndex_ = workGraphProp_->GetWorkGraphIndex(stateObject->GetProgramName().c_str());

	if (stateObject->GetStateObjectType() == StateObjectType::WorkGraphMesh) {
		workGraphProp_->SetMaximumInputRecords(workGraphIndex_, maxInputRecords, maxNodeInputs);
	}

	workGraphProp_->GetWorkGraphMemoryRequirements(workGraphIndex_, &memReqs_);
	cout << "WorkGraph memory requirements = " << memReqs_.MaxSizeInBytes << endl;
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(memReqs_.MaxSizeInBytes);
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	CD3DX12_HEAP_PROPERTIES prop(D3D12_HEAP_TYPE_DEFAULT);
	result = pDevice_->GetDevice()->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(backingMemory_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		throw std::runtime_error("Failed to CreateCommittedResource for backingmemory : " + to_string(result));
	}
	backingMemoryAddressRange_.SizeInBytes = memReqs_.MaxSizeInBytes;
	backingMemoryAddressRange_.StartAddress = backingMemory_->GetGPUVirtualAddress();
	numEntryPoints_ = workGraphProp_->GetNumEntrypoints(workGraphIndex_);
	numNodes_ = workGraphProp_->GetNumNodes(workGraphIndex_);

	// NOTE : 
	// local root argumentÇégÇ¢ÇΩÇ¢èÍçáÅCÇ±Ç±Ç≈ëÄçÏ
	//
	auto stateObjectDesc = stateObject->GetStateObjectDesc();
	if (std::holds_alternative<StateObjectDesc::WorkGraphDesc>(stateObjectDesc.typeDesc_)) {
		auto workGraphDesc = std::get<StateObjectDesc::WorkGraphDesc>(stateObjectDesc.typeDesc_);
		UINT numLocalRootSig = 0;
		UINT maxLocalRootSigSize = 0;
		for (auto& programDesc : workGraphDesc.programDescs_) {
			if (programDesc.resourceSet_) {
				numLocalRootSig++;
				auto alignedSize = AlignForLocalSigBuffer(programDesc.resourceSet_->GetRootSignature()->GetSize());
				if (maxLocalRootSigSize > alignedSize) {
					maxLocalRootSigSize = alignedSize;
				}
			}
		}
		if (maxLocalRootSigSize != 0) {
			localRootSigBuffer_ = pDevice_->CreateBuffer(BufferType::Upload, maxLocalRootSigSize, numLocalRootSig);
			void* rawPtr = localRootSigBuffer_->Map();
			for (auto& programDesc : workGraphDesc.programDescs_) {
				if (programDesc.resourceSet_) {
					auto alignedSize = AlignForLocalSigBuffer(programDesc.resourceSet_->GetRootSignature()->GetSize());
					// TODO : Copy
					uint8_t* pStartLocalSig = static_cast<uint8_t*>(rawPtr);
					auto pThisLocalSig = pStartLocalSig + 0;
					int idLocalSig = 0;
					const auto resourceSetDescs = programDesc.resourceSet_->GetResourceSetDescs();
					for (const auto& resourceSetDesc : resourceSetDescs) {
						if (std::holds_alternative<DescriptorManagerHandle>(resourceSetDesc.bindResource)) {
							auto descriptorManagerHandle = std::get<DescriptorManagerHandle>(resourceSetDesc.bindResource);
							auto add = descriptorManagerHandle->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
							pThisLocalSig += CopyMem(pThisLocalSig, &add, sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
						}
						else if (std::holds_alternative<BufferHandle>(resourceSetDesc.bindResource)) {
							auto bufferHandle = std::get<BufferHandle>(resourceSetDesc.bindResource);
							auto add = bufferHandle->GetGPUAddress();
							pThisLocalSig += CopyMem(pThisLocalSig, &add, sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
						}
						else if (std::holds_alternative<ConstantsHandle>(resourceSetDesc.bindResource)) {
							auto constants = std::get<ConstantsHandle>(resourceSetDesc.bindResource);
							// sizeof(float) = sizeof(UINT) = sizeof(int) = 4 Bytes
							pThisLocalSig += CopyMem(pThisLocalSig, constants.get(), sizeof(float) * constants->GetNumConstants());
						}
					}
				}
			}

			localRootSigBuffer_->Unmap();

			localRootArgumentsTableAddress_.SizeInBytes = maxLocalRootSigSize * numLocalRootSig;
			localRootArgumentsTableAddress_.StartAddress = localRootSigBuffer_->GetGPUAddress();
			localRootArgumentsTableAddress_.StrideInBytes = maxLocalRootSigSize;
		}
	}

	pgDesc_.Type = D3D12_PROGRAM_TYPE_WORK_GRAPH;
	pgDesc_.WorkGraph.BackingMemory = backingMemoryAddressRange_;
	pgDesc_.WorkGraph.Flags = D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE;
	pgDesc_.WorkGraph.ProgramIdentifier = workGraphProgramID_;
	// NOTE : 
	// local root argumentÇégÇ§èÍçáÇ±Ç±Ç≈ìoò^
	//pgDesc_.WorkGraph.NodeLocalRootArgumentsTable = 
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE WorkGraph::GetBackingMemoryAddressRange()
{
	return backingMemoryAddressRange_;
}

ComPtr<ID3D12Resource> WorkGraph::GetBackingMemory()
{
	return backingMemory_;
}

D3D12_PROGRAM_IDENTIFIER WorkGraph::GetProgramID()
{
	return workGraphProgramID_;
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE WorkGraph::GetLocalSigSize()
{
	return localRootArgumentsTableAddress_;
}

D3D12_SET_PROGRAM_DESC WorkGraph::GetProgramDesc() const
{
	return pgDesc_;
}

D3D12_SET_PROGRAM_DESC* WorkGraph::GetPProgramDesc()
{
	return &pgDesc_;
}