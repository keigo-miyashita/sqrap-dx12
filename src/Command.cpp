#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

bool Command::CreateCommandList(D3D12_COMMAND_LIST_TYPE commandType, wstring name)
{
	commandType_ = commandType;
	if (FAILED(pDevice_->GetDevice()->CreateCommandAllocator(commandType, IID_PPV_ARGS(commandAllocator_.ReleaseAndGetAddressOf())))) {
		return false;
	}
	wstring cmdAllocName = L"CommandAllocator";
	commandAllocator_->SetName((cmdAllocName + name).c_str());
	if (FAILED(pDevice_->GetDevice()->CreateCommandList(0, commandType, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(commandList_.ReleaseAndGetAddressOf())))) {
		return true;
	}
	wstring cmdListName = L"CommandList";
	commandList_->SetName((cmdListName + name).c_str());
	return true;
}

bool Command::InitializeStableCommandList(std::wstring name)
{
	if (FAILED(commandList_->QueryInterface(IID_PPV_ARGS(stableCommandList_.ReleaseAndGetAddressOf())))) {
		return false;
	}
	stableCommandList_->SetName((L"StableCommandList" + name).c_str());
	return true;
}

bool Command::InitializeLatestCommandList(std::wstring name)
{
	if (FAILED(commandList_->QueryInterface(IID_PPV_ARGS(latestCommandList_.ReleaseAndGetAddressOf())))) {
		return false;
	}
	latestCommandList_->SetName((L"LatestCommandList" + name).c_str());
	return true;
}

bool Command::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE commandType, wstring name)
{
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Type = commandType;
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	if (FAILED(pDevice_->GetDevice()->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(commandQueue_.ReleaseAndGetAddressOf())))) {
		return false;
	}
	wstring cmdQueueName = L"CommandQueue";
	commandQueue_->SetName((cmdQueueName	+ name).c_str());

	return true;
}

Command::Command()
{

}

bool Command::Init(Device* pDevice, D3D12_COMMAND_LIST_TYPE commandType, wstring name)
{
	pDevice_ = pDevice;
	if (!CreateCommandList(commandType, name)) {
		return false;
	}

	if (!InitializeStableCommandList(name)) {
		return false;
	}

	if (!InitializeLatestCommandList(name)) {
		return false;
	}

	if (!CreateCommandQueue(commandType, name)) {
		return false;
	}

	return true;
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

void Command::CopyBuffer(Buffer& srcBuffer, Buffer& destBuffer)
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