#include "Fence.hpp"

#include "Command.hpp"
#include "Device.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	Fence::Fence(const Device& device, QueueType queueType, std::wstring name) : device_(device), name_(name), queueType_(queueType)
	{
		HRESULT result = device_.GetDevice()->CreateFence(fenceVal_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[Fence::Fence] CreateFence failed\nHRESULT = " + to_string(result));
		}
		fence_->SetName(name_.c_str());

		hEvent_ = CreateEvent(nullptr, false, false, nullptr);
		if (hEvent_ == nullptr) {
			throw runtime_error("[Fence::Fence] CreateEvent failed");
		}
	}

	Fence::~Fence()
	{
		CloseHandle(hEvent_);
	}

	UINT64 Fence::Signal()
	{
		ID3D12CommandQueue* queue = nullptr;
		queue = device_.GetCommandQueue(queueType_).Get();

		// fenceVal is atomic, so write and read at the same time
		// so var = ++fenceVal_
		const UINT64 val = ++fenceVal_;
		HRESULT result = queue->Signal(fence_.Get(), val);
		if (FAILED(result)) {
			throw runtime_error("[Fence::Signal] Signal failed\nHRESULT = " + to_string(result));
		}
		return val;
	}

	void Fence::WaitSignal()
	{
		const UINT64 targetVal = fenceVal_.load();
		if (fence_->GetCompletedValue() < targetVal) {
			fence_->SetEventOnCompletion(targetVal, hEvent_);
			WaitForSingleObject(hEvent_, INFINITE);
		}
	}

	void Fence::WaitSignal(UINT64 fenceVal)
	{
		if (fence_->GetCompletedValue() < fenceVal) {
			fence_->SetEventOnCompletion(fenceVal, hEvent_);
			WaitForSingleObject(hEvent_, INFINITE);
		}
	}

	bool Fence::CheckSignal() const
	{
		return !(fence_->GetCompletedValue() < fenceVal_.load());
	}

	const ComPtr<ID3D12Fence>& Fence::GetFence() const
	{
		return fence_;
	}

	UINT64 Fence::GetFenceVal() const
	{
		return fenceVal_.load();
	}

	QueueType Fence::GetQueueType() const
	{
		return queueType_;
	}
}