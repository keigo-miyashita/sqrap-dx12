#pragma once

#include <common.hpp>

#define LatestCommandList ID3D12GraphicsCommandList10

class CommandList
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	// D3D12 command allocator
	ComPtr<ID3D12CommandAllocator> commandAllocator_ = nullptr;
	// D3D12 Command list type
	D3D12_COMMAND_LIST_TYPE commandType_;
	ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;
	ComPtr<LatestCommandList> latestCommandList_ = nullptr;

	bool CreateCommandList(const ComPtr<ID3D12Device>& device, D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT, std::wstring name = L"CommandList");
	bool InitializeLatestCommandList();


public:
	CommandList();
	~CommandList() = default;
	bool Init(const ComPtr<ID3D12Device>& device, D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT, std::wstring name = L"CommandList");
	D3D12_COMMAND_LIST_TYPE GetCommandType();
	ComPtr<ID3D12CommandAllocator> GetCommandAllocator();
	ComPtr<ID3D12GraphicsCommandList> GetCommandList();
	ComPtr<LatestCommandList> GetLatestCommandList();
};