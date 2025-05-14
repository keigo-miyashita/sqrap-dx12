#pragma once

#include <common.hpp>

class CommandQueue
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	// D3D12 command allocator
	ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;
	// D3D12 Command list type
	D3D12_COMMAND_LIST_TYPE commandType_;

	bool CreateCommandQueue(const ComPtr<ID3D12Device>& device, D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT);


public:
	CommandQueue();
	~CommandQueue() = default;
	bool Init(const ComPtr<ID3D12Device>& device, D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT);
	D3D12_COMMAND_LIST_TYPE GetCommandType();
	ComPtr<ID3D12CommandQueue> GetCommandQueue();
};