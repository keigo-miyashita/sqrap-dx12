#pragma once

#include "pch.hpp"

#include "Device.hpp"

#include "Alias.hpp"

namespace sqrp
{
	using StableCommandList = ID3D12GraphicsCommandList9;
	using LatestCommandList = ID3D12GraphicsCommandList10;

	class Buffer;
	class ComputePipeline;
	class Constants;
	class DescriptorHeap;
	class DescriptorManager;
	class Device;
	class Fence;
	class GraphicsPipeline;
	class Gui;
	class Indirect;
	class Mesh;
	class Resource;
	class ResourceSet;
	class RootSignature;
	class SwapChain;

	class Command
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const Device* pDevice_ = nullptr;
		std::wstring name_;
		D3D12_COMMAND_LIST_TYPE commandType_;
		QueueType queueType_ = QueueType::Graphics;
		std::vector<ComPtr<ID3D12CommandAllocator>> commandAllocators_;
		ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;
		ComPtr<StableCommandList> stableCommandList_ = nullptr;
		ComPtr<LatestCommandList> latestCommandList_ = nullptr;
		std::vector<UINT64> fenceValues_;
		UINT frameIndex_ = 0;
		UINT frameCount_ = 1;

		void CreateCommandList();
		void InitializeStableCommandList();
		void InitializeLatestCommandList();
		void Close();
		const ComPtr<ID3D12CommandAllocator>& GetCurrentCommandAllocator() const;

	public:
		Command(const Device& device, std::wstring name, D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT, UINT frameCount = 1);
		Command(const Command&) = delete;
		Command& operator=(const Command&) = delete;
		void BeginRender(const SwapChainHandle& swapchain);
		void EndRender(const SwapChainHandle& swapchain);
		void AddDrawIndexed(const MeshBaseHandle& mesh, UINT numInstances);
		void AddDrawIndexedLine(const MeshHandle& mesh, UINT numInstances);
		void Barrier(UINT numBarriers, D3D12_RESOURCE_BARRIER* pBarriers);
		void Barrier(std::vector<D3D12_RESOURCE_BARRIER> barriers);
		void CopyBuffer(const ResourceHandle& srcResource, D3D12_RESOURCE_STATES srcCurrentState, const ResourceHandle& destResource, D3D12_RESOURCE_STATES destCurrentState);
		void CopyBufferRegion(const BufferHandle& srcBuffer, UINT64 srcOffset, D3D12_RESOURCE_STATES srcCurrentState, const BufferHandle& destBuffer, UINT64 destOffset, D3D12_RESOURCE_STATES destCurrentState, UINT64 numBytes);
		void Dispatch(UINT threadX, UINT threadY, UINT threadZ);
		void DispatchRays(const RaytracingHandle& raytracing);
		void DrawIndirect(const MeshHandle& mesh, const IndirectHandle& indirect, const BufferHandle& buffer, UINT maxCommandNum);
		void DrawGui(const GuiHandle& gui);
		void ExecuteIndirect(const IndirectHandle& indirect);
		void SetMesh(const MeshBaseHandle& mesh);
		void SetComputeResource(const RootSignatureHandle& computeRootSig);
		void SetComputeRoot32BitConstants(UINT rootParamIndex, const ConstantsHandle& constant);
		void SetComputeRootDescriptorTable(UINT rootParamIndex, const DescriptorManagerHandle& descManager);
		void SetComputeRootSig(const RootSignatureHandle& computeRootSig);
		void SetDescriptorHeap(const DescriptorManagerHandle& descManager);
		void SetGraphicsResource(const RootSignatureHandle& graphicsRootSig);
		void SetGraphicsRoot32BitConstants(UINT rootParamIndex, const ConstantsHandle& constant);
		void SetGraphicsRootDescriptorTable(UINT rootParamIndex, const DescriptorManagerHandle& descManager);
		void SetGraphicsRootSig(const RootSignatureHandle& graphicsRootSig);
		void SetRaytracingResource(const RootSignatureHandle& raytracingRootSig);
		void SetPipeline(const GraphicsPipelineHandle& graphicsPipeline);
		void SetPipeline(const ComputePipelineHandle& computePipeline);
		void SetPipeline(const MeshPipelineHandle& meshPipeline);
		void SetRaytracingState(const StateObjectHandle& stateObject);
		SyncPoint Submit();
		void WaitCommand();
		// "init" prefixed methods perform initialization and internally invoke WaitCommand to ensure readiness.
		void InitDataToBuffer(const BufferHandle& buffer, void* pData, UINT strideSize, UINT numElement);
		void InitDataToTexture(const TextureHandle& texture, void* pData, UINT strideSize, UINT width, UINT height);

		const ComPtr<ID3D12GraphicsCommandList>& GetCommandList() const;
		const ComPtr<StableCommandList>& GetStableCommandList() const;
		const ComPtr<LatestCommandList>& GetLatestCommandList() const;
		QueueType GetQueueType() const;
		UINT GetFrameIndex() const;
		UINT GetFrameCount() const;
	};
}