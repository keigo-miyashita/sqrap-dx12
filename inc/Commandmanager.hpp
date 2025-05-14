#pragma once

#include <common.hpp>

#define LatestCommandList ID3D12GraphicsCommandList10

class Device;
class Mesh;

class CommandManager
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
	ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;

	bool CreateCommandList(const Device& device, D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT, std::wstring name = L"Direct");
	bool InitializeLatestCommandList(std::wstring name = L"Direct");
	bool CreateCommandQueue(const Device& device, D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT, std::wstring name = L"Direct");


public:
	CommandManager();
	~CommandManager() = default;
	bool Init(const Device& device, D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT, std::wstring name = L"Direct");
	void AddDrawIndexed(const Mesh& mesh, UINT numInstances);

	D3D12_COMMAND_LIST_TYPE GetCommandType();
	ComPtr<ID3D12CommandAllocator> GetCommandAllocator() const;
	ComPtr<ID3D12GraphicsCommandList> GetCommandList() const;
	ComPtr<LatestCommandList> GetLatestCommandList() const;
	ComPtr<ID3D12CommandQueue> GetCommandQueue() const;
};