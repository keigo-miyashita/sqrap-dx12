#pragma once

#include <common.hpp>

#define StableCommandList ID3D12GraphicsCommandList9
#define LatestCommandList ID3D12GraphicsCommandList10

class Buffer;
class ComputePipeline;
class DescriptorHeap;
class DescriptorManager;
class Device;
class Fence;
class GraphicsPipeline;
class GUI;
class Indirect;
class Mesh;
class RootSignature;

class Command
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const Device* pDevice_ = nullptr;
	D3D12_COMMAND_LIST_TYPE commandType_;
	std::wstring name_;
	// D3D12 command allocator
	ComPtr<ID3D12CommandAllocator> commandAllocator_ = nullptr;
	// D3D12 Command list type
	ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;
	ComPtr<StableCommandList> stableCommandList_ = nullptr;
	ComPtr<LatestCommandList> latestCommandList_ = nullptr;
	ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;
	std::shared_ptr<Fence> fence_;

	bool CreateCommandList();
	bool InitializeStableCommandList();
	bool InitializeLatestCommandList();
	bool CreateCommandQueue();


public:
	Command(const Device& device, D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT, std::wstring name = L"");
	~Command() = default;
	//bool Init(Device* pDevice, D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT, std::wstring name = L"Direct");
	void AddDrawIndexed(const Mesh& mesh, UINT numInstances);
	void AddDrawIndexedLine(const Mesh& mesh, UINT numInstances);
	void Barrier(UINT numBarriers, D3D12_RESOURCE_BARRIER* pBarriers);
	void CopyBuffer(Buffer& srcBuffer, Buffer& destBuffer);
	void CopyBufferRegion(Buffer& srcBuffer, UINT srcOffset, Buffer& destBuffer, UINT destOffset, UINT numBytes);
	void DrawIndirect(const Mesh& mesh, const Indirect& indirect, const Buffer& buffer, UINT maxCommandNum);
	void Dispatch(UINT threadX, UINT threadY, UINT threadZ);
	void DrawGUI(GUI& GUI);
	void SetPipeline(const GraphicsPipeline& graphicsPipeline);
	void SetPipeline(const ComputePipeline& computePipeline);
	void SetGraphicsRootSig(const RootSignature& graphicsRootSig);
	void SetComputeRootSig(const RootSignature& computeRootSig);
	void SetDescriptorHeap(const DescriptorManager& descManager);
	void SetGraphicsRootDescriptorTable(UINT rootParamIndex, const DescriptorManager& descManager);
	void SetComputeRootDescriptorTable(UINT rootParamIndex, const DescriptorManager& descManager);
	void SetGraphicsRoot32BitConstants(UINT rootParamIndex, UINT num32bitsConstant, void* pData);
	void WaitCommand();

	D3D12_COMMAND_LIST_TYPE GetCommandType();
	ComPtr<ID3D12CommandAllocator> GetCommandAllocator() const;
	ComPtr<ID3D12GraphicsCommandList> GetCommandList() const;
	ComPtr<StableCommandList> GetStableCommandList() const;
	ComPtr<LatestCommandList> GetLatestCommandList() const;
	ComPtr<ID3D12CommandQueue> GetCommandQueue() const;
	Fence& GetFence();
};