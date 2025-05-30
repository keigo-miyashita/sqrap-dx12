#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

//bool WorkGraph::InitWorkGraphContext(const StateObject& stateObject, UINT maxInputRecords, UINT maxNodeInputs)
//{
//	ComPtr<ID3D12StateObjectProperties1> soProp = nullptr;
//	auto result = stateObject.GetStateObject()->QueryInterface(IID_PPV_ARGS(soProp.ReleaseAndGetAddressOf()));
//	if (FAILED(result)) {
//		cerr << "Failed to get StateObjectProperties1" << endl;
//		return false;
//	}
//
//	workGraphProgramID_ = soProp->GetProgramIdentifier(stateObject.GetProgramName().c_str());
//
//	stateObject.GetStateObject()->QueryInterface(IID_PPV_ARGS(workGraphProp_.ReleaseAndGetAddressOf()));
//	workGraphIndex_ = workGraphProp_->GetWorkGraphIndex(stateObject.GetProgramName().c_str());
//
//	if (stateObject.GetStateObjectType() == StateObjectType::WorkGraphMesh) {
//		workGraphProp_->SetMaximumInputRecords(workGraphIndex_, maxInputRecords, maxNodeInputs);
//	}
//	workGraphProp_->GetWorkGraphMemoryRequirements(workGraphIndex_, &memReqs_);
//	cout << "WorkGraph memory requirements = " << memReqs_.MaxSizeInBytes << endl;
//	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(memReqs_.MaxSizeInBytes);
//	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
//	CD3DX12_HEAP_PROPERTIES prop(D3D12_HEAP_TYPE_DEFAULT);
//	result = pDevice_->GetDevice()->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(backingMemory_.ReleaseAndGetAddressOf()));
//	if (FAILED(result)) {
//		cerr << "Failed to CreateCommittedResource for backingmemory" << endl;
//	}
//	backingMemoryAddressRange_.SizeInBytes = memReqs_.MaxSizeInBytes;
//	backingMemoryAddressRange_.StartAddress = backingMemory_->GetGPUVirtualAddress();
//	numEntryPoints_ = workGraphProp_->GetNumEntrypoints(workGraphIndex_);
//	numNodes_ = workGraphProp_->GetNumNodes(workGraphIndex_);
//
//	// NOTE : 
//	// local root argumentÇégÇ¢ÇΩÇ¢èÍçáÅCÇ±Ç±Ç≈ëÄçÏ
//	//
//
//	pgDesc_.Type = D3D12_PROGRAM_TYPE_WORK_GRAPH;
//	pgDesc_.WorkGraph.BackingMemory = backingMemoryAddressRange_;
//	pgDesc_.WorkGraph.Flags = D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE;
//	pgDesc_.WorkGraph.ProgramIdentifier = workGraphProgramID_;
//	// NOTE : 
//	// local root argumentÇégÇ§èÍçáÇ±Ç±Ç≈ìoò^
//	//pgDesc_.WorkGraph.NodeLocalRootArgumentsTable = 
//
//	return true;
//}

WorkGraph::WorkGraph(const Device& device, const StateObject& stateObject, UINT maxInputRecords, UINT maxNodeInputs, std::wstring name)
	: pDevice_(&device), name_(name)
{
	ComPtr<ID3D12StateObjectProperties1> soProp = nullptr;
	auto result = stateObject.GetStateObject()->QueryInterface(IID_PPV_ARGS(soProp.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		throw std::runtime_error("Failed to QueryInterface for StateObjectProperties1 : " + to_string(result));
	}

	workGraphProgramID_ = soProp->GetProgramIdentifier(stateObject.GetProgramName().c_str());

	stateObject.GetStateObject()->QueryInterface(IID_PPV_ARGS(workGraphProp_.ReleaseAndGetAddressOf()));
	workGraphIndex_ = workGraphProp_->GetWorkGraphIndex(stateObject.GetProgramName().c_str());

	if (stateObject.GetStateObjectType() == StateObjectType::WorkGraphMesh) {
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

	pgDesc_.Type = D3D12_PROGRAM_TYPE_WORK_GRAPH;
	pgDesc_.WorkGraph.BackingMemory = backingMemoryAddressRange_;
	pgDesc_.WorkGraph.Flags = D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE;
	pgDesc_.WorkGraph.ProgramIdentifier = workGraphProgramID_;
	// NOTE : 
	// local root argumentÇégÇ§èÍçáÇ±Ç±Ç≈ìoò^
	//pgDesc_.WorkGraph.NodeLocalRootArgumentsTable = 
}

//bool WorkGraph::Init(Device* pDevice, const StateObject& stateObject, UINT maxInputRecords, UINT maxNodeInputs)
//{
//	pDevice_ = pDevice;
//	if (pDevice_ == nullptr) {
//		cerr << "WorkGraph class doesn't have any pointer" << endl;
//		return false;
//	}
//	if (!InitWorkGraphContext(stateObject)) {
//		cerr << "Failed to InitWorkGraphContext" << endl;
//		return false;
//	}
//	return true;
//}

D3D12_SET_PROGRAM_DESC WorkGraph::GetProgramDesc() const
{
	return pgDesc_;
}

D3D12_SET_PROGRAM_DESC* WorkGraph::GetPProgramDesc()
{
	return &pgDesc_;
}