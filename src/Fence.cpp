#include "Fence.hpp"

#include "Command.hpp"
#include "Device.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

Fence::Fence(const Device& device, std::wstring name) : pDevice_(&device), name_(name)
{
	HRESULT result = pDevice_->GetDevice()->CreateFence(fenceVal_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		throw std::runtime_error("Failed to CreateFence : " + to_string(result));
	}
	fence_->SetName(name_.c_str());
}

bool Fence::WaitCommand(Command& command)
{
	command.GetCommandList()->Close();

	ID3D12CommandList* cmdLists[] = { command.GetCommandList().Get() };
	command.GetCommandQueue()->ExecuteCommandLists(1, cmdLists);
	HRESULT result = command.GetCommandQueue()->Signal(fence_.Get(), ++fenceVal_);

	if (FAILED(result)) {
		return false;
	}

	if (fence_->GetCompletedValue() < fenceVal_) {
		auto event = CreateEvent(nullptr, false, false, nullptr);
		fence_->SetEventOnCompletion(fenceVal_, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}

	command.GetCommandAllocator()->Reset();
	command.GetCommandList()->Reset(command.GetCommandAllocator().Get(), nullptr);
}

ComPtr<ID3D12Fence> Fence::GetFence()
{
	return fence_;
}

UINT64 Fence::GetFenceVal()
{
	return fenceVal_;
}