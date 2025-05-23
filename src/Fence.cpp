#include <common.hpp>
#include <comdef.h>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

bool Fence::CreateFence(wstring name)
{
	if (FAILED(pDevice_->GetDevice()->CreateFence(fenceVal_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.ReleaseAndGetAddressOf())))) {
		return false;
	}
	fence_->SetName(name.c_str());
	return true;
}

Fence::Fence()
{

}

bool Fence::Init(Device* pDevice, wstring name)
{
	pDevice_ = pDevice;
	if (!CreateFence(name)) {
		return false;
	}

	return true;
}

void Fence::WaitCommand(Command& command)
{
	command.GetCommandList()->Close();

	ID3D12CommandList* cmdLists[] = { command.GetCommandList().Get() };
	command.GetCommandQueue()->ExecuteCommandLists(1, cmdLists);
	HRESULT result = command.GetCommandQueue()->Signal(fence_.Get(), ++fenceVal_);

	if (FAILED(result)) {
		_com_error err(result);
		wcerr << L"Error message: " << err.ErrorMessage() << endl;
		result = pDevice_->GetDevice()->GetDeviceRemovedReason();
		_com_error DeviceRemovedReason(result);
		wcerr << L"Device removed reason: " << DeviceRemovedReason.ErrorMessage() << endl;
		cerr << "Failed to Signal" << endl;
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