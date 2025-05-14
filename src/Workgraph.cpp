#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

bool WorkGraph::InitWorkGraphContext(const Device& device, const StateObject& stateObject)
{
	ComPtr<ID3D12StateObjectProperties1> soProp = nullptr;
	auto result = stateObject.GetStateObject()->QueryInterface(IID_PPV_ARGS(soProp.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		cerr << "Failed to get StateObjectProperties1" << endl;
		return false;
	}

	workGraphProgramID_ = soProp->GetProgramIdentifier(stateObject.GetProgramName().c_str());

	stateObject.GetStateObject()->QueryInterface(IID_PPV_ARGS(workGraphProp_.ReleaseAndGetAddressOf()));
	workGraphIndex_ = workGraphProp_->GetWorkGraphIndex(stateObject.GetProgramName().c_str());

	workGraphProp_->GetWorkGraphMemoryRequirements(workGraphIndex_, &memReqs_);
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(memReqs_.MaxSizeInBytes);
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	CD3DX12_HEAP_PROPERTIES prop(D3D12_HEAP_TYPE_DEFAULT);
	result = device.GetDevice()->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(backingMemory_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		cerr << "Failed to CreateCommittedResource for backingmemory" << endl;
	}
	backingMemoryAddressRange_.SizeInBytes = memReqs_.MaxSizeInBytes;
	backingMemoryAddressRange_.StartAddress = backingMemory_->GetGPUVirtualAddress();
	numEntryPoints_ = workGraphProp_->GetNumEntrypoints(workGraphIndex_);
	numNodes_ = workGraphProp_->GetNumNodes(workGraphIndex_);

	// NOTE : 
	// local root argumentÇégÇ¢ÇΩÇ¢èÍçáÅCÇ±Ç±Ç≈ëÄçÏ
	//

	pgDesc_.Type = D3D12_PROGRAM_TYPE_WORK_GRAPH;
	pgDesc_.WorkGraph.BackingMemory = backingMemoryAddressRange_;
	pgDesc_.WorkGraph.Flags = D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE;
	pgDesc_.WorkGraph.ProgramIdentifier = workGraphProgramID_;
	// NOTE : 
	// local root argumentÇégÇ§èÍçáÇ±Ç±Ç≈ìoò^
	//pgDesc_.WorkGraph.NodeLocalRootArgumentsTable = 

	return true;
}

WorkGraph::WorkGraph()
{

}

bool WorkGraph::Init(const Device& device, const StateObject& stateObject)
{
	return true;
}

D3D12_SET_PROGRAM_DESC WorkGraph::GetProgramDesc() const
{
	return pgDesc_;
}