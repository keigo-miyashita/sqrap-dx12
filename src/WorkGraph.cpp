#include "WorkGraph.hpp"

#include "Device.hpp"
#include "Resource.hpp"
#include "Pipeline.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	UINT WorkGraph::AlignForLocalSigBuffer(UINT size)
	{
		const UINT alignment = 8; // sizeof(D3D12_GPU_VIRTUAL_ADDRESS), sizeof(D3D12_GPU_DESCRIPTOR_HANDLE)
		return (size + (alignment - 1)) & ~(alignment - 1);
	}

	UINT WorkGraph::CopyMem(void* dest, const void* data, UINT size)
	{
		memcpy(dest, data, size);

		return size;
	}

	D3D12_SET_PROGRAM_DESC WorkGraph::MakeSetProgramDesc(bool initialize) const
	{
		D3D12_SET_PROGRAM_DESC pgDesc = {};

		pgDesc.Type = D3D12_PROGRAM_TYPE_WORK_GRAPH;
		pgDesc.WorkGraph.BackingMemory = backingMemoryAddressRange_;
		pgDesc.WorkGraph.Flags = initialize ? D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE : D3D12_SET_WORK_GRAPH_FLAG_NONE;
		pgDesc.WorkGraph.ProgramIdentifier = workGraphProgramID_;
		// ローカルルート引数が無いときは範囲 0（null）のまま渡す
		pgDesc.WorkGraph.NodeLocalRootArgumentsTable = localRootArgumentsTableAddress_;

		return pgDesc;
	}

	WorkGraph::WorkGraph(const Device& device, wstring name, const StateObjectHandle& stateObject,
		std::vector<std::vector<LocalRootArguments>> computeLocalRootArguments,
		std::vector<std::vector<LocalRootArguments>> graphicsLocalRootArguments,
		UINT maxInputRecords, UINT maxNodeInputs)
		: name_(name)
	{
		if (stateObject->GetStateObjectType() == StateObjectType::Raytracing) {
			throw runtime_error("[WorkGraph::WorkGraph] stateObject is not a workgraph state object");
		}

		const auto& computeNodes = stateObject->GetComputeNodes();
		const auto& graphicsNodes = stateObject->GetGraphicsNodes();

		if (computeLocalRootArguments.empty()) {
			computeLocalRootArguments.resize(computeNodes.size());
		}
		if (graphicsLocalRootArguments.empty()) {
			graphicsLocalRootArguments.resize(graphicsNodes.size());
		}

		if (computeLocalRootArguments.size() != computeNodes.size()) {
			throw runtime_error("[WorkGraph::WorkGraph] computeLocalRootArguments size does not match the number of compute nodes (pass {} if no compute node uses local root arguments)");
		}
		if (graphicsLocalRootArguments.size() != graphicsNodes.size()) {
			throw runtime_error("[WorkGraph::WorkGraph] graphicsLocalRootArguments size does not match the number of graphics nodes (pass {} if no graphics node uses local root arguments)");
		}

		ComPtr<ID3D12StateObjectProperties1> soProp = nullptr;
		auto result = stateObject->GetStateObject()->QueryInterface(IID_PPV_ARGS(soProp.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[WorkGraph::WorkGraph] QueryInterface for StateObjectProperties1 failed\nHRESULT = " + to_string(result));
		}

		workGraphProgramID_ = soProp->GetProgramIdentifier(stateObject->GetWorkGraphProgramName().c_str());

		stateObject->GetStateObject()->QueryInterface(IID_PPV_ARGS(workGraphProp_.ReleaseAndGetAddressOf()));
		workGraphIndex_ = workGraphProp_->GetWorkGraphIndex(stateObject->GetWorkGraphProgramName().c_str());

		if (stateObject->HasGraphicsNodes()) {
			if (maxInputRecords == 0 || maxNodeInputs == 0) {
				throw runtime_error("[WorkGraph::WorkGraph] maxInputRecords and maxNodeInputs must be greater than 0 for a work graph with mesh nodes");
			}
			workGraphProp_->SetMaximumInputRecords(workGraphIndex_, maxInputRecords, maxNodeInputs);
		} else if (maxInputRecords != 0 || maxNodeInputs != 0) {
			cout << "[WorkGraph::WorkGraph] maxInputRecords and maxNodeInputs are ignored for a work graph without mesh nodes." << endl;
		}

		workGraphProp_->GetWorkGraphMemoryRequirements(workGraphIndex_, &memReqs_);
		cout << "WorkGraph memory requirements = " << memReqs_.MaxSizeInBytes << endl;
		CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(memReqs_.MaxSizeInBytes);
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		CD3DX12_HEAP_PROPERTIES prop(D3D12_HEAP_TYPE_DEFAULT);
		result = device.GetDevice()->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(backingMemory_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[WorkGraph::WorkGraph] CreateCommittedResource for backing memory failed\nHRESULT = " + to_string(result));
		}
		backingMemory_->SetName((name_ + L"_WorkGraph_BackingMemory").c_str());
		backingMemoryAddressRange_.SizeInBytes = memReqs_.MaxSizeInBytes;
		backingMemoryAddressRange_.StartAddress = backingMemory_->GetGPUVirtualAddress();
		numEntryPoints_ = workGraphProp_->GetNumEntrypoints(workGraphIndex_);
		numNodes_ = workGraphProp_->GetNumNodes(workGraphIndex_);

		vector<pair<D3D12_NODE_ID, IndexInWorkGraphDesc>> computeNodeLRSNames;
		vector<pair<D3D12_NODE_ID, IndexInWorkGraphDesc>> graphicsNodeLRSNames;

		// ローカルルート引数テーブル：ノードごとに 1 レコード、stride は最大のローカルルートシグネチャのサイズ（8 の倍数）
		UINT maxLocalRootSigSize = 0;
		for (int i = 0; i < computeNodes.size(); i++) {
			const auto& computeNode = computeNodes[i];
			if (computeNode.localRootSignature_) {
				auto alignedSize = AlignForLocalSigBuffer(computeNode.localRootSignature_->GetSize());
				if (maxLocalRootSigSize < alignedSize) {
					maxLocalRootSigSize = alignedSize;
				}
				computeNodeLRSNames.push_back(make_pair(D3D12_NODE_ID(computeNode.nodeName_.c_str(), computeNode.nodeIndex_), i));
			}
			else if (computeLocalRootArguments[i].size() > 0) {
				throw runtime_error("[WorkGraph::WorkGraph] computeLocalRootArguments[" + to_string(i) + "] is not empty, but computeNodeDescs_[" + to_string(i) + "] does not have a local root signature");
			}
		}
		for (int i = 0; i < graphicsNodes.size(); i++) {
			const auto& graphicsNode = graphicsNodes[i];
			if (graphicsNode.localRootSignature_) {
				auto alignedSize = AlignForLocalSigBuffer(graphicsNode.localRootSignature_->GetSize());
				if (maxLocalRootSigSize < alignedSize) {
					maxLocalRootSigSize = alignedSize;
				}
				graphicsNodeLRSNames.push_back(make_pair(D3D12_NODE_ID(graphicsNode.nodeName_.c_str(), graphicsNode.nodeIndex_), i));
			}
			else if (graphicsLocalRootArguments[i].size() > 0) {
				throw runtime_error("[WorkGraph::WorkGraph] graphicsLocalRootArguments[" + to_string(i) + "] is not empty, but graphicsProgramDescs_[" + to_string(i) + "] does not have a local root signature");
			}
		}

		UINT maxTableIndex = 0;
		if (maxLocalRootSigSize != 0) {
			for (UINT nodeIndex = 0; nodeIndex < numNodes_; nodeIndex++) {
				UINT tableIndex = workGraphProp_->GetNodeLocalRootArgumentsTableIndex(workGraphIndex_, nodeIndex);
				if (tableIndex == (UINT)(-1)) {
					continue;
				}
				if (tableIndex > maxTableIndex) {
					maxTableIndex = tableIndex;
				}
			}
		}

		if (maxLocalRootSigSize != 0) {
			localRootSigBuffer_ = device.CreateBuffer(L"_WorkGraph_localRootSig", BufferType::Upload, maxLocalRootSigSize, maxTableIndex + 1);
			void* rawPtr = localRootSigBuffer_->Map();
			uint8_t* pStartLocalSig = static_cast<uint8_t*>(rawPtr);
			D3D12_NODE_ID id = {};
			auto existNode = [&](const pair<D3D12_NODE_ID, IndexInWorkGraphDesc>& a)
			{
					return wcscmp(a.first.Name, id.Name) == 0 && a.first.ArrayIndex == id.ArrayIndex;
			};
			for (UINT nodeIndex = 0; nodeIndex < numNodes_; nodeIndex++) {
				UINT tableIndex = workGraphProp_->GetNodeLocalRootArgumentsTableIndex(workGraphIndex_, nodeIndex);
				if (tableIndex == (UINT)(-1)) {
					continue;
				}
				auto pThisLocalSig = pStartLocalSig + maxLocalRootSigSize * tableIndex;
				id = workGraphProp_->GetNodeID(workGraphIndex_, nodeIndex);
				auto computeCandidate = find_if(computeNodeLRSNames.begin(), computeNodeLRSNames.end(), existNode);
				auto graphicsCandidate = find_if(graphicsNodeLRSNames.begin(), graphicsNodeLRSNames.end(), existNode);
				if (computeCandidate == computeNodeLRSNames.end() && graphicsCandidate == graphicsNodeLRSNames.end()) {
					throw runtime_error("[WorkGraph::WorkGraph] a node with a local root signature matches no nodeName_ / nodeIndex_ in the work graph desc");
				}
				if (computeCandidate != computeNodeLRSNames.end() && graphicsCandidate != graphicsNodeLRSNames.end()) {
					throw runtime_error("[WorkGraph::WorkGraph] a node matches both a compute node and a graphics node in the work graph desc");
				}
				if (computeCandidate != computeNodeLRSNames.end()) {
					const UINT indexInWorkGraphDesc = computeCandidate->second;
					const auto& localRootArguments = computeLocalRootArguments[indexInWorkGraphDesc];
					const auto& localRootSignature = computeNodes[indexInWorkGraphDesc].localRootSignature_;
					if (localRootArguments.size() != localRootSignature->GetRootParametersVec().size()) {
						throw runtime_error("[WorkGraph::WorkGraph] computeLocalRootArguments[" + to_string(indexInWorkGraphDesc) + "] size does not match the number of root parameters of its local root signature");
					}
					for (UINT j = 0; j < localRootArguments.size(); j++) {
						localRootSignature->ValidateRootArgument(j, localRootArguments[j]);
					}
					RootSignature::WriteLocalRootArguments(pThisLocalSig, localRootArguments);
				}
				if (graphicsCandidate != graphicsNodeLRSNames.end()) {
					const UINT indexInWorkGraphDesc = graphicsCandidate->second;
					const auto& localRootArguments = graphicsLocalRootArguments[indexInWorkGraphDesc];
					const auto& localRootSignature = graphicsNodes[indexInWorkGraphDesc].localRootSignature_;
					if (localRootArguments.size() != localRootSignature->GetRootParametersVec().size()) {
						throw runtime_error("[WorkGraph::WorkGraph] graphicsLocalRootArguments[" + to_string(indexInWorkGraphDesc) + "] size does not match the number of root parameters of its local root signature");
					}
					for (UINT j = 0; j < localRootArguments.size(); j++) {
						localRootSignature->ValidateRootArgument(j, localRootArguments[j]);
					}
					RootSignature::WriteLocalRootArguments(pThisLocalSig, localRootArguments);
				}
				
			}

			localRootSigBuffer_->Unmap();

			localRootArgumentsTableAddress_.SizeInBytes = localRootSigBuffer_->GetStrideSize() * localRootSigBuffer_->GetNumElement();
			localRootArgumentsTableAddress_.StartAddress = localRootSigBuffer_->GetGPUAddress();
			localRootArgumentsTableAddress_.StrideInBytes = maxLocalRootSigSize;
		}
	}

	D3D12_GPU_VIRTUAL_ADDRESS_RANGE WorkGraph::GetBackingMemoryAddressRange() const
	{
		return backingMemoryAddressRange_;
	}

	const ComPtr<ID3D12Resource>& WorkGraph::GetBackingMemory() const
	{
		return backingMemory_;
	}

	D3D12_PROGRAM_IDENTIFIER WorkGraph::GetProgramID() const
	{
		return workGraphProgramID_;
	}

	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE WorkGraph::GetLocalRootArgumentsTable() const
	{
		return localRootArgumentsTableAddress_;
	}
}
