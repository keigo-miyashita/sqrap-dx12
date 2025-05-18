#pragma once

#include <common.hpp>

class Device;

class Fence
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	Device* pDevice_ = nullptr;
	// D3D12 fence
	ComPtr<ID3D12Fence> fence_ = nullptr;
	// Fence value
	UINT64 fenceVal_ = 0;

	bool CreateFence(std::wstring = L"Fence");


public:
	Fence();
	~Fence() = default;
	bool Init(Device* pDevice, std::wstring = L"Fence");
	void WaitCommand(CommandManager& commandManager);

	ComPtr<ID3D12Fence> GetFence();
	UINT64 GetFenceVal();
};