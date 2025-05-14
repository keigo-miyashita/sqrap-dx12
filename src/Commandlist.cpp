#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

bool CommandList::CreateCommandList(const ComPtr<ID3D12Device>& device, D3D12_COMMAND_LIST_TYPE commandType, wstring name)
{
	commandType_ = commandType;
	if (FAILED(device->CreateCommandAllocator(commandType, IID_PPV_ARGS(commandAllocator_.ReleaseAndGetAddressOf())))) {
		return false;
	}
	wstring nameCommandAllocator = L"CommandAllocator";
	commandAllocator_->SetName((nameCommandAllocator + name).c_str());
	if (FAILED(device->CreateCommandList(0, commandType, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(commandList_.ReleaseAndGetAddressOf())))) {
		return false;
	}
	commandList_->SetName(name.c_str());
	return true;
}

bool CommandList::InitializeLatestCommandList()
{
	if (FAILED(commandList_->QueryInterface(IID_PPV_ARGS(latestCommandList_.ReleaseAndGetAddressOf())))) {
		return false;
	}
	latestCommandList_->SetName(L"Latest Commandlist");
	return true;
}

CommandList::CommandList()
{

}

bool CommandList::Init(const ComPtr<ID3D12Device>& device, D3D12_COMMAND_LIST_TYPE commandType, wstring name)
{
	if (!CreateCommandList(device, commandType, name)) {
		return false;
	}

	if (!InitializeLatestCommandList()) {
		return false;
	}

	return true;
}

D3D12_COMMAND_LIST_TYPE CommandList::GetCommandType()
{
	return commandType_;
}

ComPtr<ID3D12CommandAllocator> CommandList::GetCommandAllocator()
{
	return commandAllocator_;
}

ComPtr<ID3D12GraphicsCommandList> CommandList::GetCommandList()
{
	return commandList_;
}

ComPtr<LatestCommandList> CommandList::GetLatestCommandList()
{
	return latestCommandList_;
}