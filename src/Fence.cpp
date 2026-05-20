#include "Fence.hpp"

#include "Command.hpp"
#include "Device.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	Fence::Fence(const Device& device, wstring name) : pDevice_(&device), name_(name)
	{
		HRESULT result = pDevice_->GetDevice()->CreateFence(fenceVal_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("Failed to CreateFence : " + to_string(result));
		}
		fence_->SetName(name_.c_str());

		hEvent_ = CreateEvent(nullptr, false, false, nullptr);
		if (hEvent_ == nullptr) {
			throw runtime_error("Failed to CreateEvent for fence");
		}
	}

	Fence::~Fence()
	{
		CloseHandle(hEvent_);
	}

	bool Fence::WaitCommand(Command& command, QueueType queueType)
	{
		command.GetCommandList()->Close();

		ID3D12CommandList* cmdLists[] = { command.GetCommandList().Get() };
		HRESULT result;
		if (queueType == QueueType::Graphics) {
			pDevice_->GetGraphicsCommandQueue()->ExecuteCommandLists(1, cmdLists);
			result = pDevice_->GetGraphicsCommandQueue()->Signal(fence_.Get(), ++fenceVal_);
		}
		else if (queueType == QueueType::Compute) {
			pDevice_->GetComputeCommandQueue()->ExecuteCommandLists(1, cmdLists);
			result = pDevice_->GetComputeCommandQueue()->Signal(fence_.Get(), ++fenceVal_);
		}

		if (FAILED(result)) {
			return false;
		}

		if (fence_->GetCompletedValue() < fenceVal_) {
			fence_->SetEventOnCompletion(fenceVal_, hEvent_);
			WaitForSingleObject(hEvent_, INFINITE);
		}

		command.GetCommandAllocator()->Reset();
		command.GetCommandList()->Reset(command.GetCommandAllocator().Get(), nullptr);

		return true;
	}

	bool Fence::Signal(QueueType queueType)
	{
		HRESULT result;
		if (queueType == QueueType::Graphics) {
			result = pDevice_->GetGraphicsCommandQueue()->Signal(fence_.Get(), ++fenceVal_);
		}
		else if (queueType == QueueType::Compute) {
			result = pDevice_->GetComputeCommandQueue()->Signal(fence_.Get(), ++fenceVal_);
		}

		if (FAILED(result)) {
			return false;
		}
		return true;
	}
	void Fence::WaitSignal()
	{
		if (fence_->GetCompletedValue() < fenceVal_) {
			fence_->SetEventOnCompletion(fenceVal_, hEvent_);
			WaitForSingleObject(hEvent_, INFINITE);
		}
	}

	bool Fence::CheckSignal()
	{
		auto completedVal = fence_->GetCompletedValue();

		return !(completedVal < fenceVal_);
	}

	ComPtr<ID3D12Fence> Fence::GetFence() const
	{
		return fence_;
	}

	UINT64 Fence::GetFenceVal() const
	{
		return fenceVal_;
	}
}