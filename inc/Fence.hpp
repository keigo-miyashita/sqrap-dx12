#pragma once

#include <common.hpp>

#include <Command.hpp>

class Command;
class Device;

class Fence
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const Device* pDevice_ = nullptr;
	std::wstring name_;
	// D3D12 fence
	ComPtr<ID3D12Fence> fence_ = nullptr;
	// Fence value
	UINT64 fenceVal_ = 0;

	bool CreateFence();


public:
	Fence(const Device& device, std::wstring name = L"");
	~Fence() = default;
	bool WaitCommand(Command& command);

	ComPtr<ID3D12Fence> GetFence();
	UINT64 GetFenceVal();
};