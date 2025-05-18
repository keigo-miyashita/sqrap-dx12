#pragma once

#include <common.hpp>

#define StableCommandList ID3D12GraphicsCommandList9
#define LatestCommandList ID3D12GraphicsCommandList10

class Buffer;
class Device;
class Indirect;
class Mesh;

class CommandManager
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	Device* pDevice_ = nullptr;
	// D3D12 command allocator
	ComPtr<ID3D12CommandAllocator> commandAllocator_ = nullptr;
	// D3D12 Command list type
	D3D12_COMMAND_LIST_TYPE commandType_;
	ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;
	ComPtr<StableCommandList> stableCommandList_ = nullptr;
	ComPtr<LatestCommandList> latestCommandList_ = nullptr;
	ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;

	bool CreateCommandList(D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT, std::wstring name = L"Direct");
	bool InitializeStableCommandList(std::wstring name = L"Direct");
	bool InitializeLatestCommandList(std::wstring name = L"Direct");
	bool CreateCommandQueue(D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT, std::wstring name = L"Direct");


public:
	CommandManager();
	~CommandManager() = default;
	bool Init(Device* pDevice, D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT, std::wstring name = L"Direct");
	void AddDrawIndexed(const Mesh& mesh, UINT numInstances);
	void CopyBuffer(Buffer& srcBuffer, Buffer& destBuffer);
	void CopyBufferRegion(Buffer& srcBuffer, UINT srcOffset, Buffer& destBuffer, UINT destOffset, UINT numBytes);
	void DrawIndirect(const Mesh& mesh, const Indirect& indirect, const Buffer& buffer, UINT maxCommandNum);

	D3D12_COMMAND_LIST_TYPE GetCommandType();
	ComPtr<ID3D12CommandAllocator> GetCommandAllocator() const;
	ComPtr<ID3D12GraphicsCommandList> GetCommandList() const;
	ComPtr<StableCommandList> GetStableCommandList() const;
	ComPtr<LatestCommandList> GetLatestCommandList() const;
	ComPtr<ID3D12CommandQueue> GetCommandQueue() const;
};