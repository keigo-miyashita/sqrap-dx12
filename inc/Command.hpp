#pragma once

#include "pch.hpp"

#include "Alias.hpp"

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
class Resource;
class ResourceSet;
class RootSignature;

struct Constants;

class Command
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const Device* pDevice_ = nullptr;
	D3D12_COMMAND_LIST_TYPE commandType_;
	std::wstring name_;
	ComPtr<ID3D12CommandAllocator> commandAllocator_ = nullptr;
	ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;
	ComPtr<StableCommandList> stableCommandList_ = nullptr;
	ComPtr<LatestCommandList> latestCommandList_ = nullptr;
	ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;
	FenceHandle fence_;

	bool CreateCommandList();
	bool InitializeStableCommandList();
	bool InitializeLatestCommandList();
	bool CreateCommandQueue();


public:
	Command(const Device& device, D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT, std::wstring name = L"");
	~Command() = default;
	void AddDrawIndexed(MeshHandle mesh, UINT numInstances);
	void AddDrawIndexedLine(MeshHandle mesh, UINT numInstances);
	void Barrier(UINT numBarriers, D3D12_RESOURCE_BARRIER* pBarriers);
	void CopyBuffer(ResourceHandle srcResource, ResourceHandle destResource);
	void CopyBufferRegion(BufferHandle srcBuffer, UINT srcOffset, BufferHandle destBuffer, UINT destOffset, UINT numBytes);
	void DrawIndirect(MeshHandle mesh, IndirectHandle indirect, BufferHandle buffer, UINT maxCommandNum);
	void Dispatch(UINT threadX, UINT threadY, UINT threadZ);
	void DrawGUI(GUI& GUI);
	void SetPipeline(GraphicsPipelineHandle graphicsPipeline);
	void SetPipeline(ComputePipelineHandle computePipeline);
	void SetGraphicsRootSig(RootSignatureHandle graphicsRootSig);
	void SetComputeRootSig(RootSignatureHandle computeRootSig);
	void SetDescriptorHeap(DescriptorManagerHandle descManager);
	void SetGraphicsRootDescriptorTable(UINT rootParamIndex, DescriptorManagerHandle descManager);
	void SetComputeRootDescriptorTable(UINT rootParamIndex, DescriptorManagerHandle descManager);
	void SetGraphicsRoot32BitConstants(UINT rootParamIndex, UINT num32bitsConstant, void* pData);
	void SetComputeRoot32BitConstants(UINT rootParamIndex, UINT num32bitsConstant, void* pData);
	void SetComputeResourceSet(std::shared_ptr<ResourceSet> resourceSet);
	void SetGraphicsResourceSet(std::shared_ptr<ResourceSet> resourceSet);
	bool WaitCommand();

	D3D12_COMMAND_LIST_TYPE GetCommandType();
	ComPtr<ID3D12CommandAllocator> GetCommandAllocator() const;
	ComPtr<ID3D12GraphicsCommandList> GetCommandList() const;
	ComPtr<StableCommandList> GetStableCommandList() const;
	ComPtr<LatestCommandList> GetLatestCommandList() const;
	ComPtr<ID3D12CommandQueue> GetCommandQueue() const;
	Fence& GetFence();
};