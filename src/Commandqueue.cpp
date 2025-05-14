#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

bool CommandQueue::CreateCommandQueue(const ComPtr<ID3D12Device>& device, D3D12_COMMAND_LIST_TYPE commandType)
{
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Type = commandType;
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	if (FAILED(device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(commandQueue_.ReleaseAndGetAddressOf())))) {
		return false;
	}

	return true;
}

CommandQueue::CommandQueue()
{

}

bool CommandQueue::Init(const ComPtr<ID3D12Device>& device, D3D12_COMMAND_LIST_TYPE commandType)
{
	if (!CreateCommandQueue(device, commandType)) {
		return false;
	}

	return true;
}

D3D12_COMMAND_LIST_TYPE CommandQueue::GetCommandType()
{
	return commandType_;
}

ComPtr<ID3D12CommandQueue> CommandQueue::GetCommandQueue()
{
	return commandQueue_;
}