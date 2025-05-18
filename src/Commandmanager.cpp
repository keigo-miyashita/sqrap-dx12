#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

bool CommandManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE commandType, wstring name)
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

bool CommandManager::InitializeStableCommandList(std::wstring name)
{
	if (FAILED(commandList_->QueryInterface(IID_PPV_ARGS(stableCommandList_.ReleaseAndGetAddressOf())))) {
		return false;
	}
	stableCommandList_->SetName((L"StableCommandList" + name).c_str());
	return true;
}

bool CommandManager::InitializeLatestCommandList(std::wstring name)
{
	if (FAILED(commandList_->QueryInterface(IID_PPV_ARGS(latestCommandList_.ReleaseAndGetAddressOf())))) {
		return false;
	}
	latestCommandList_->SetName((L"LatestCommandList" + name).c_str());
	return true;
}

bool CommandManager::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE commandType, wstring name)
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

CommandManager::CommandManager()
{

}

bool CommandManager::Init(Device* pDevice, D3D12_COMMAND_LIST_TYPE commandType, wstring name)
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

void CommandManager::AddDrawIndexed(const Mesh& mesh, UINT numInstances)
{
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList_->IASetVertexBuffers(0, 1, mesh.GetVBViewPtr());
	commandList_->IASetIndexBuffer(mesh.GetIBViewPtr());
	commandList_->DrawIndexedInstanced(mesh.GetNumIndices(), 1, 0, 0, 0);
}

void CommandManager::CopyBuffer(Buffer& srcBuffer, Buffer& destBuffer)
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

	commandList_->ResourceBarrier(rscBarriers.size(), rscBarriers.data());
	commandList_->CopyResource(destBuffer.GetResource().Get(), srcBuffer.GetResource().Get());

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(destBuffer.GetResource().Get(), D3D12_RESOURCE_STATE_COPY_DEST, destBuffer.GetResourceState());
	commandList_->ResourceBarrier(1, &barrier);
}

void CommandManager::CopyBufferRegion(Buffer& srcBuffer, UINT srcOffset, Buffer& destBuffer, UINT destOffset, UINT numBytes)
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

	commandList_->ResourceBarrier(rscBarriers.size(), rscBarriers.data());
	commandList_->CopyBufferRegion(destBuffer.GetResource().Get(), 0, srcBuffer.GetResource().Get(), 0, numBytes);

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(destBuffer.GetResource().Get(), D3D12_RESOURCE_STATE_COPY_DEST, destBuffer.GetResourceState());
	commandList_->ResourceBarrier(1, &barrier);
}

void CommandManager::DrawIndirect(const Mesh& mesh, const Indirect& indirect, const Buffer& buffer, UINT maxCommandNum)
{
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList_->IASetVertexBuffers(0, 1, mesh.GetVBViewPtr());
	commandList_->IASetIndexBuffer(mesh.GetIBViewPtr());
	commandList_->ExecuteIndirect(indirect.GetCommandSignature().Get(), maxCommandNum, buffer.GetResource().Get(), 0, nullptr, 0);
}

void CommandManager::Dispatch(UINT threadX, UINT threadY, UINT threadZ)
{
	commandList_->Dispatch(threadX, threadY, threadZ);
}

D3D12_COMMAND_LIST_TYPE CommandManager::GetCommandType()
{
	return commandType_;
}

ComPtr<ID3D12CommandAllocator> CommandManager::GetCommandAllocator() const
{
	return commandAllocator_;
}

ComPtr<ID3D12GraphicsCommandList> CommandManager::GetCommandList() const
{
	return commandList_;
}

ComPtr<StableCommandList> CommandManager::GetStableCommandList() const
{
	return stableCommandList_;
}

ComPtr<LatestCommandList> CommandManager::GetLatestCommandList() const
{
	return latestCommandList_;
}

ComPtr<ID3D12CommandQueue> CommandManager::GetCommandQueue() const
{
	return commandQueue_;
}