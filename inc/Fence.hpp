#pragma once

#include "pch.hpp"

#include "Device.hpp"

namespace sqrp
{
	class Command;

	class Fence
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const Device& device_;
		std::wstring name_;
		ComPtr<ID3D12Fence> fence_ = nullptr;
		std::atomic<UINT64> fenceVal_ = 0;
		HANDLE hEvent_ = nullptr;
		QueueType queueType_ = QueueType::Graphics;

	public:
		Fence(const Device& device, QueueType queueType, std::wstring name = L"");
		~Fence();
		Fence(const Fence&) = delete;
		Fence& operator=(const Fence&) = delete;
		UINT64 Signal();
		void WaitSignal(); // Stop CPU until GPU reaches the fence value.
		void WaitSignal(UINT64 fenceVal); // Stop CPU until GPU reaches the fence value.
		bool CheckSignal() const;// Query the fence value (CPU does not stop)

		const ComPtr<ID3D12Fence>& GetFence() const;
		UINT64 GetFenceVal() const;
		QueueType GetQueueType() const;
	};
}