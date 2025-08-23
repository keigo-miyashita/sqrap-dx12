#pragma once

#include "pch.hpp"

#include "Device.hpp"

#include "Alias.hpp"

#define StableCommandList ID3D12GraphicsCommandList9
#define LatestCommandList ID3D12GraphicsCommandList10

namespace sqrp
{
	class Buffer;
	class ComputePipeline;
	class Constants;
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
		//ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;
		FenceHandle fence_;

		void CreateCommandList();
		void InitializeStableCommandList();
		void InitializeLatestCommandList();
		//void CreateCommandQueue();

	public:
		Command(const Device& device, D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT, std::wstring name = L"");
		~Command() = default;
		void AddDrawIndexed(MeshHandle mesh, UINT numInstances);
		void AddDrawIndexedLine(MeshHandle mesh, UINT numInstances);
		void Barrier(UINT numBarriers, D3D12_RESOURCE_BARRIER* pBarriers);
		void CopyBuffer(ResourceHandle srcResource, ResourceHandle destResource);
		void CopyBufferRegion(BufferHandle srcBuffer, UINT srcOffset, BufferHandle destBuffer, UINT destOffset, UINT numBytes);
		void Dispatch(UINT threadX, UINT threadY, UINT threadZ);
		void DispatchRays(RayTracingHandle rayTracing);
		void DrawIndirect(MeshHandle mesh, IndirectHandle indirect, BufferHandle buffer, UINT maxCommandNum);
		void DrawGUI(GUIHandle GUI);
		void ExecuteIndirect(IndirectHandle indirect);
		void SetMesh(MeshHandle mesh);
		void SetComputeResource(RootSignatureHandle computeRootSig);
		void SetComputeRoot32BitConstants(UINT rootParamIndex, ConstantsHandle constant);
		void SetComputeRootDescriptorTable(UINT rootParamIndex, DescriptorManagerHandle descManager);
		void SetComputeRootSig(RootSignatureHandle computeRootSig);
		void SetDescriptorHeap(DescriptorManagerHandle descManager);
		void SetGraphicsResource(RootSignatureHandle graphicsRootSig);
		void SetGraphicsRoot32BitConstants(UINT rootParamIndex, ConstantsHandle constant);
		void SetGraphicsRootDescriptorTable(UINT rootParamIndex, DescriptorManagerHandle descManager);
		void SetGraphicsRootSig(RootSignatureHandle graphicsRootSig);
		void SetPipeline(GraphicsPipelineHandle graphicsPipeline);
		void SetPipeline(ComputePipelineHandle computePipeline);
		void SetRayTracingState(StateObjectHandle stateObject);
		bool WaitCommand(QueueType queueType = QueueType::Graphics);

		// "init" prefixed methods perform initialization and internally invoke WaitCommand to ensure readiness.
		void InitDataToBuffer(BufferHandle buffer, void* pData, UINT strideSize, UINT numElement);

		D3D12_COMMAND_LIST_TYPE GetCommandType();
		ComPtr<ID3D12CommandAllocator> GetCommandAllocator() const;
		ComPtr<ID3D12GraphicsCommandList> GetCommandList() const;
		ComPtr<StableCommandList> GetStableCommandList() const;
		ComPtr<LatestCommandList> GetLatestCommandList() const;
		//ComPtr<ID3D12CommandQueue> GetCommandQueue() const;
		Fence& GetFence();
	};
}