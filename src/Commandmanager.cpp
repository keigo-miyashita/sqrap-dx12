#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

bool CommandManager::CreateCommandList(const Device& device, D3D12_COMMAND_LIST_TYPE commandType, wstring name)
{
	commandType_ = commandType;
	if (FAILED(device.GetDevice()->CreateCommandAllocator(commandType, IID_PPV_ARGS(commandAllocator_.ReleaseAndGetAddressOf())))) {
		return false;
	}
	wstring cmdAllocName = L"CommandAllocator";
	commandAllocator_->SetName((cmdAllocName + name).c_str());
	if (FAILED(device.GetDevice()->CreateCommandList(0, commandType, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(commandList_.ReleaseAndGetAddressOf())))) {
		return true;
	}
	wstring cmdListName = L"CommandList";
	commandList_->SetName((cmdListName + name).c_str());
	return true;
}

bool CommandManager::InitializeLatestCommandList(std::wstring name)
{
	if (FAILED(commandList_->QueryInterface(IID_PPV_ARGS(latestCommandList_.ReleaseAndGetAddressOf())))) {
		return false;
	}
	wstring latestName = L"LatestCommandList";
	latestCommandList_->SetName((latestName + name).c_str());
	return true;
}

bool CommandManager::CreateCommandQueue(const Device& device, D3D12_COMMAND_LIST_TYPE commandType, wstring name)
{
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Type = commandType;
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	if (FAILED(device.GetDevice()->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(commandQueue_.ReleaseAndGetAddressOf())))) {
		return false;
	}
	wstring cmdQueueName = L"CommandQueue";
	commandQueue_->SetName((cmdQueueName	+ name).c_str());

	return true;
}

CommandManager::CommandManager()
{

}

bool CommandManager::Init(const Device& device, D3D12_COMMAND_LIST_TYPE commandType, wstring name)
{
	if (!CreateCommandList(device, commandType, name)) {
		return false;
	}

	if (!InitializeLatestCommandList(name)) {
		return false;
	}

	if (!CreateCommandQueue(device, commandType, name)) {
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

ComPtr<LatestCommandList> CommandManager::GetLatestCommandList() const
{
	return latestCommandList_;
}

ComPtr<ID3D12CommandQueue> CommandManager::GetCommandQueue() const
{
	return commandQueue_;
}