#pragma once

#include "pch.hpp"

#include "Device.hpp"

namespace sqrp
{
	class Command;
	class Device;

	class Fence
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const Device* pDevice_ = nullptr;
		std::wstring name_;
		ComPtr<ID3D12Fence> fence_ = nullptr;
		UINT64 fenceVal_ = 0;

	public:
		Fence(const Device& device, std::wstring name = L"");
		~Fence() = default;
		bool WaitCommand(Command& command, QueueType queueType = QueueType::Graphics);

		ComPtr<ID3D12Fence> GetFence();
		UINT64 GetFenceVal();
	};
}