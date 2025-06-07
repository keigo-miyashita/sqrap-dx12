#include "Command.hpp"

#include "Descriptormanager.hpp"
#include "Device.hpp"
#include "Fence.hpp"
#include "Indirect.hpp"
#include "Gui.hpp"
#include "Mesh.hpp"
#include "Pipeline.hpp"
#include "Resource.hpp"
#include "Rootsignature.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

bool Command::CreateCommandList()
{
	commandType_ = commandType_;
	HRESULT result = pDevice_->GetDevice()->CreateCommandAllocator(commandType_, IID_PPV_ARGS(commandAllocator_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		throw std::runtime_error("Failed to CreateCommandAllocator : " + to_string(result));
		return false;
	}
	wstring cmdAllocName = L"CommandAllocator";
	commandAllocator_->SetName((cmdAllocName + name_).c_str());
	result = pDevice_->GetDevice()->CreateCommandList(0, commandType_, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(commandList_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		throw std::runtime_error("Failed to CreateCommandList : " + to_string(result));
		return true;
	}
	wstring cmdListName = L"CommandList";
	commandList_->SetName((cmdListName + name_).c_str());
	return true;
}

bool Command::InitializeStableCommandList()
{
	HRESULT result = commandList_->QueryInterface(IID_PPV_ARGS(stableCommandList_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		throw std::runtime_error("Failed to QueryInterface for StableCommandList : " + to_string(result));
		return false;
	}
	stableCommandList_->SetName((L"StableCommandList" + name_).c_str());
	return true;
}

bool Command::InitializeLatestCommandList()
{
	HRESULT result = commandList_->QueryInterface(IID_PPV_ARGS(latestCommandList_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		throw std::runtime_error("Failed to QueryInterface for LatestCommandList : " + to_string(result));
		return false;
	}
	latestCommandList_->SetName((L"LatestCommandList" + name_).c_str());
	return true;
}

bool Command::CreateCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Type = commandType_;
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	HRESULT result = pDevice_->GetDevice()->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(commandQueue_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		throw std::runtime_error("Failed to CreateCommandQueue : " + to_string(result));
		return false;
	}
	wstring cmdQueueName = L"CommandQueue";
	commandQueue_->SetName((cmdQueueName	+ name_).c_str());

	return true;
}

Command::Command(const Device& device, D3D12_COMMAND_LIST_TYPE commandType, wstring name) : pDevice_(&device), commandType_(commandType), name_(name)
{
	CreateCommandList();

	InitializeStableCommandList();

	InitializeLatestCommandList();

	CreateCommandQueue();

	fence_ = pDevice_->CreateFence(name);
}

void Command::AddDrawIndexed(const Mesh& mesh, UINT numInstances)
{
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList_->IASetVertexBuffers(0, 1, mesh.GetVBViewPtr());
	commandList_->IASetIndexBuffer(mesh.GetIBViewPtr());
	commandList_->DrawIndexedInstanced(mesh.GetNumIndices(), 1, 0, 0, 0);
}

void Command::AddDrawIndexedLine(const Mesh& mesh, UINT numInstances)
{
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	commandList_->IASetVertexBuffers(0, 1, mesh.GetVBViewPtr());
	commandList_->IASetIndexBuffer(mesh.GetIBViewPtr());
	commandList_->DrawIndexedInstanced(mesh.GetNumIndices(), 1, 0, 0, 0);
}

void Command::Barrier(UINT numBarriers, D3D12_RESOURCE_BARRIER* pBarriers)
{
	commandList_->ResourceBarrier(numBarriers, pBarriers);
}

void Command::CopyBuffer(Resource& srcBuffer, Resource& destBuffer)
{
	vector<CD3DX12_RESOURCE_BARRIER> rscBarriers;
	if (srcBuffer.GetResourceState() != D3D12_RESOURCE_STATE_COPY_SOURCE) {
		auto srcBarrier = CD3DX12_RESOURCE_BARRIER::Transition(srcBuffer.GetResource().Get(), srcBuffer.GetResourceState(), D3D12_RESOURCE_STATE_COPY_SOURCE);
		rscBarriers.push_back(srcBarrier);
	}
	if (destBuffer.GetResourceState() != D3D12_RESOURCE_STATE_COPY_DEST) {
		auto destBarrier = CD3DX12_RESOURCE_BARRIER::Transition(destBuffer.GetResource().Get(), destBuffer.GetResourceState(), D3D12_RESOURCE_STATE_COPY_DEST);
		rscBarriers.push_back(destBarrier);
	}

	if (rscBarriers.size() != 0) {
		commandList_->ResourceBarrier(rscBarriers.size(), rscBarriers.data());
	}
	commandList_->CopyResource(destBuffer.GetResource().Get(), srcBuffer.GetResource().Get());

	rscBarriers.clear();
	if (srcBuffer.GetResourceState() != D3D12_RESOURCE_STATE_COPY_SOURCE) {
		auto srcbarrier = CD3DX12_RESOURCE_BARRIER::Transition(srcBuffer.GetResource().Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, srcBuffer.GetResourceState());
		rscBarriers.push_back(srcbarrier);
	}
	if (destBuffer.GetResourceState() != D3D12_RESOURCE_STATE_COPY_DEST) {
		auto destbarrier = CD3DX12_RESOURCE_BARRIER::Transition(destBuffer.GetResource().Get(), D3D12_RESOURCE_STATE_COPY_DEST, destBuffer.GetResourceState());
		rscBarriers.push_back(destbarrier);
	}
	if (rscBarriers.size() != 0) {
		commandList_->ResourceBarrier(rscBarriers.size(), rscBarriers.data());
	}
}

void Command::CopyBufferRegion(Buffer& srcBuffer, UINT srcOffset, Buffer& destBuffer, UINT destOffset, UINT numBytes)
{
	vector<CD3DX12_RESOURCE_BARRIER> rscBarriers;
	if (srcBuffer.GetResourceState() != D3D12_RESOURCE_STATE_COPY_SOURCE) {
		auto srcBarrier = CD3DX12_RESOURCE_BARRIER::Transition(srcBuffer.GetResource().Get(), srcBuffer.GetResourceState(), D3D12_RESOURCE_STATE_COPY_SOURCE);
		srcBuffer.SetResourceState(D3D12_RESOURCE_STATE_COPY_SOURCE);
		rscBarriers.push_back(srcBarrier);
	}
	if (destBuffer.GetResourceState() != D3D12_RESOURCE_STATE_COPY_DEST) {
		auto destBarrier = CD3DX12_RESOURCE_BARRIER::Transition(destBuffer.GetResource().Get(), destBuffer.GetResourceState(), D3D12_RESOURCE_STATE_COPY_DEST);
		rscBarriers.push_back(destBarrier);
	}

	if (rscBarriers.size() != 0) {
		commandList_->ResourceBarrier(rscBarriers.size(), rscBarriers.data());
	}
	commandList_->CopyBufferRegion(destBuffer.GetResource().Get(), destOffset, srcBuffer.GetResource().Get(), srcOffset, numBytes);

	rscBarriers.clear();
	if (srcBuffer.GetResourceState() != D3D12_RESOURCE_STATE_COPY_SOURCE) {
		auto srcbarrier = CD3DX12_RESOURCE_BARRIER::Transition(srcBuffer.GetResource().Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, srcBuffer.GetResourceState());
		rscBarriers.push_back(srcbarrier);
	}
	if (destBuffer.GetResourceState() != D3D12_RESOURCE_STATE_COPY_DEST) {
		auto destbarrier = CD3DX12_RESOURCE_BARRIER::Transition(destBuffer.GetResource().Get(), D3D12_RESOURCE_STATE_COPY_DEST, destBuffer.GetResourceState());
		rscBarriers.push_back(destbarrier);
	}
	if (rscBarriers.size() != 0) {
		commandList_->ResourceBarrier(rscBarriers.size(), rscBarriers.data());
	}
}

void Command::DrawIndirect(const Mesh& mesh, const Indirect& indirect, const Buffer& buffer, UINT maxCommandNum)
{
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList_->IASetVertexBuffers(0, 1, mesh.GetVBViewPtr());
	commandList_->IASetIndexBuffer(mesh.GetIBViewPtr());
	commandList_->ExecuteIndirect(indirect.GetCommandSignature().Get(), maxCommandNum, buffer.GetResource().Get(), 0, nullptr, 0);
}

void Command::Dispatch(UINT threadX, UINT threadY, UINT threadZ)
{
	commandList_->Dispatch(threadX, threadY, threadZ);
}

void Command::DrawGUI(GUI& GUI)
{
	commandList_->SetDescriptorHeaps(1, GUI.GetImguiDescHeap().GetAddressOf());
	GUI.Draw(*this);
}

void Command::SetPipeline(const GraphicsPipeline& graphicsPipeline)
{
	commandList_->SetPipelineState(graphicsPipeline.GetPipelineState().Get());
}

void Command::SetPipeline(const ComputePipeline& computePipeline)
{
	commandList_->SetPipelineState(computePipeline.GetPipelineState().Get());
}

void Command::SetGraphicsRootSig(const RootSignature& graphicsRootSig)
{
	commandList_->SetGraphicsRootSignature(graphicsRootSig.GetRootSignature().Get());
}

void Command::SetComputeRootSig(const RootSignature& computeRootSig)
{
	commandList_->SetComputeRootSignature(computeRootSig.GetRootSignature().Get());
}

void Command::SetDescriptorHeap(const DescriptorManager& descManager)
{
	commandList_->SetDescriptorHeaps(1, descManager.GetDescriptorHeap().GetAddressOf());
}

void Command::SetGraphicsRootDescriptorTable(UINT rootParamIndex, const DescriptorManager& descManager)
{
	commandList_->SetGraphicsRootDescriptorTable(rootParamIndex, descManager.GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
}

void Command::SetComputeRootDescriptorTable(UINT rootParamIndex, const DescriptorManager& descManager)
{
	commandList_->SetComputeRootDescriptorTable(rootParamIndex, descManager.GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
}

void Command::SetGraphicsRoot32BitConstants(UINT rootParamIndex, UINT num32bitsConstant, void* pData)
{
	commandList_->SetGraphicsRoot32BitConstants(rootParamIndex, num32bitsConstant, pData, 0);
}

void Command::SetComputeRoot32BitConstants(UINT rootParamIndex, UINT num32bitsConstant, void* pData)
{
	commandList_->SetComputeRoot32BitConstants(rootParamIndex, num32bitsConstant, pData, 0);
}

void Command::SetComputeResourceSet(std::shared_ptr<ResourceSet> resourceSet)
{
	const std::vector<BindResource> resources = resourceSet->GetBindedResources();
	const std::vector<std::shared_ptr<DescriptorManager>> descManagers = resourceSet->GetDescManagers();

	commandList_->SetComputeRootSignature(resourceSet->GetRootSignature()->GetRootSignature().Get());

	std::vector<ID3D12DescriptorHeap*> rawHeaps;
	for (auto& h : descManagers) {
		rawHeaps.push_back(h->GetDescriptorHeap().Get());
	}

	commandList_->SetDescriptorHeaps(rawHeaps.size(), rawHeaps.data());;

	UINT rootParamIndex = 0;
	for (const BindResource& resource : resources) {
		if (std::holds_alternative<D3D12_GPU_DESCRIPTOR_HANDLE>(resource)) {
			D3D12_GPU_DESCRIPTOR_HANDLE dh = std::get<D3D12_GPU_DESCRIPTOR_HANDLE>(resource);
			commandList_->SetComputeRootDescriptorTable(rootParamIndex, dh);
			rootParamIndex++;
		}
		else if (std::holds_alternative<D3D12_GPU_VIRTUAL_ADDRESS>(resource)) {
			D3D12_GPU_VIRTUAL_ADDRESS add = std::get<D3D12_GPU_VIRTUAL_ADDRESS>(resource);
			// NOTE : Add bind descriptor
			rootParamIndex++;
		}
		else if (std::holds_alternative<Constants>(resource)) {
			Constants cs = std::get<Constants>(resource);
			/*float color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
			cs.constants = static_cast<void*>(color);*/
			commandList_->SetComputeRoot32BitConstants(rootParamIndex, cs.numConstants, cs.constants, cs.numOffset);
			float* constants = static_cast<float*>(cs.constants);
			cout << "cs = " << constants[0] << " " << constants[1] << " " << constants[2] << " " << constants[3] << endl;
			cout << "cs = " << cs.numConstants << " " << cs.numOffset << endl;
			rootParamIndex++;
		}
	}
}

void Command::SetGraphicsResourceSet(std::shared_ptr<ResourceSet> resourceSet)
{
	const std::vector<BindResource> resources = resourceSet->GetBindedResources();
	const std::vector<std::shared_ptr<DescriptorManager>> descManagers = resourceSet->GetDescManagers();

	commandList_->SetGraphicsRootSignature(resourceSet->GetRootSignature()->GetRootSignature().Get());

	// NOTE : Using raw ptr (memory leaks may occur)
	std::vector<ID3D12DescriptorHeap*> rawHeaps;
	for (auto& h : descManagers) {
		rawHeaps.push_back(h->GetDescriptorHeap().Get());
	}

	commandList_->SetDescriptorHeaps(rawHeaps.size(), rawHeaps.data());;

	UINT rootParamIndex = 0;
	for (const BindResource& resource : resources) {
		if (std::holds_alternative<D3D12_GPU_DESCRIPTOR_HANDLE>(resource)) {
			D3D12_GPU_DESCRIPTOR_HANDLE dh = std::get<D3D12_GPU_DESCRIPTOR_HANDLE>(resource);
			commandList_->SetGraphicsRootDescriptorTable(rootParamIndex, dh);
			rootParamIndex++;
		}
		else if (std::holds_alternative<D3D12_GPU_VIRTUAL_ADDRESS>(resource)) {
			D3D12_GPU_VIRTUAL_ADDRESS add = std::get<D3D12_GPU_VIRTUAL_ADDRESS>(resource);
			// NOTE : Add bind descriptor
			rootParamIndex++;
		}
		else if (std::holds_alternative<Constants>(resource)) {
			Constants cs = std::get<Constants>(resource);
			commandList_->SetGraphicsRoot32BitConstants(rootParamIndex, cs.numConstants, cs.constants, cs.numOffset);
			rootParamIndex++;
		}
	}
}

bool Command::WaitCommand()
{
	if (!fence_->WaitCommand(*this)) {
		return false;
	}
	return true;
}

D3D12_COMMAND_LIST_TYPE Command::GetCommandType()
{
	return commandType_;
}

ComPtr<ID3D12CommandAllocator> Command::GetCommandAllocator() const
{
	return commandAllocator_;
}

ComPtr<ID3D12GraphicsCommandList> Command::GetCommandList() const
{
	return commandList_;
}

ComPtr<StableCommandList> Command::GetStableCommandList() const
{
	return stableCommandList_;
}

ComPtr<LatestCommandList> Command::GetLatestCommandList() const
{
	return latestCommandList_;
}

ComPtr<ID3D12CommandQueue> Command::GetCommandQueue() const
{
	return commandQueue_;
}

Fence& Command::GetFence()
{
	return *fence_;
}