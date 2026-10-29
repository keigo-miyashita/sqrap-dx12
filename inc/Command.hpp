#pragma once

#include "pch.hpp"

#include "Device.hpp"
#include "TimelinePoint.hpp"

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
	class RootSignature;
	class SwapChain;

	class Command
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const Device& device_;
		std::wstring name_;
		D3D12_COMMAND_LIST_TYPE commandType_ = D3D12_COMMAND_LIST_TYPE_DIRECT;
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
		void SetResource(PipelineBindPoint type, const RootSignatureHandle& rootSig, std::initializer_list<RootArguments> bindingResources);
		const ComPtr<ID3D12CommandAllocator>& GetCurrentCommandAllocator() const;

	public:
		Command(const Device& device, std::wstring name, D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT, UINT frameCount = 1);
		Command(const Command&) = delete;
		Command& operator=(const Command&) = delete;
		void BeginRender(const SwapChainHandle& swapchain);
		void EndRender(const SwapChainHandle& swapchain);
		void AddDrawIndexed(const MeshHandle& mesh, UINT numInstances);
		void AddDrawIndexedLine(const MeshHandle& mesh, UINT numInstances);
		void Barrier(UINT numBarriers, D3D12_RESOURCE_BARRIER* pBarriers);
		void Barrier(std::vector<D3D12_RESOURCE_BARRIER> barriers);
		void CopyBuffer(const ResourceHandle& srcResource, const ResourceHandle& destResource);
		void CopyBufferRegion(const BufferHandle& srcBuffer, UINT64 srcOffset, const BufferHandle& destBuffer, UINT64 destOffset, UINT64 numBytes);
		void Dispatch(UINT threadX, UINT threadY, UINT threadZ);
		void DispatchRays(const RaytracingHandle& raytracing, UINT width, UINT height, UINT depth, UINT rayGenIndex = 0);
		void DrawGui(const GuiHandle& gui, const SwapChainHandle& swapChain);
		void ExecuteIndirect(const IndirectHandle& indirect, const BufferHandle& argumentBuffer, const BufferHandle& counterBuffer, UINT maxCommandCount);
		void SetMesh(const MeshHandle& mesh);
		void SetComputeResource(const RootSignatureHandle& computeRootSig, std::initializer_list<RootArguments> bindingResources);
		void SetDescriptorHeap();
		void SetGraphicsResource(const RootSignatureHandle& graphicsRootSig, std::initializer_list<RootArguments> bindingResources);
		void SetPipeline(const PipelineHandle& pipeline);
		void SetRaytracingState(const StateObjectHandle& stateObject);
		void SetProgram(const WorkGraphHandle& workGraph, bool isInitialize = true);
		bool MakeTransition(const ResourceHandle& resource, D3D12_RESOURCE_STATES after, D3D12_RESOURCE_BARRIER& barrier);
		void Transition(const ResourceHandle& resource, D3D12_RESOURCE_STATES after);
		void Transition(std::initializer_list<std::pair<ResourceHandle, D3D12_RESOURCE_STATES>> transitions);
		void UAVBarrier(const ResourceHandle& resource);
		TimelinePoint Submit();
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