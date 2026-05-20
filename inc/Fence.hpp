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

		const Device* pDevice_ = nullptr;
		std::wstring name_;
		ComPtr<ID3D12Fence> fence_ = nullptr;
		UINT64 fenceVal_ = 0;
		HANDLE hEvent_ = nullptr;

	public:
		Fence(const Device& device, std::wstring name = L"");
		~Fence();
		bool WaitCommand(Command& command, QueueType queueType = QueueType::Graphics);
		bool Signal(QueueType queueType = QueueType::Graphics);
		void WaitSignal(); // Stop CPU until GPU reaches the fence value.
		bool CheckSignal();// Query the fence value (CPU does not stop)

		ComPtr<ID3D12Fence> GetFence() const;
		UINT64 GetFenceVal() const;
	};
}