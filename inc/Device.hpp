#pragma once

#include "pch.hpp"

#include "AS.hpp"
#include "DeferredRelease.hpp"
#include "Indirect.hpp"
#include "Mesh.hpp"
#include "Pipeline.hpp"
#include "Raytracing.hpp"
#include "Resource.hpp"
#include "RootSignature.hpp"
#include "WorkGraph.hpp"

#include "Alias.hpp"

namespace sqrp
{
	using StableDevice = ID3D12Device13;
	using LatestDevice = ID3D12Device14;

	void DebugOutputFormatString(const char* format, ...);

	class Command;
	class DescriptorLayout;
	class DescriptorManager;
	class Fence;
	class Gui;
	class Shader;
	class SwapChain;
	class TimelinePoint;

	enum class QueueType {
		Graphics, Compute
	};

	// Device はこれから作るすべてのハンドル（Create* が返すもの）より長く生きる必要がある。
	// Create* が付ける遅延破棄のデリータが Device を覚えており、Command / Fence / Gui / Resource / SwapChain は
	// const Device& をメンバに持つため。利用側は Device を値メンバとして、ハンドルより先に宣言する
	// （C++ のメンバは宣言と逆の順に壊れるので、先に宣言した Device が最後に壊れる）
	class Device
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;
		D3D_FEATURE_LEVEL featureLevel_ = D3D_FEATURE_LEVEL_12_1;
		ComPtr<ID3D12Device> device_ = nullptr;
		ComPtr<IDXGIAdapter4> adapter_ = nullptr;
		ComPtr<StableDevice> stableDevice_ = nullptr;
		ComPtr<LatestDevice> latestDevice_ = nullptr;
		ComPtr<ID3D12CommandQueue> graphicsCommandQueue_ = nullptr;
		ComPtr<ID3D12CommandQueue> computeCommandQueue_ = nullptr;
		FenceHandle graphicsFence_ = nullptr;
		FenceHandle computeFence_ = nullptr;
		mutable std::atomic<bool> hasComputeCommand_ = false;
		ComPtr<ID3D12DescriptorHeap> resourceDescHeap_ = nullptr;
		mutable std::atomic<UINT> resourceDescFrontIndex_ = 0;
		ComPtr<ID3D12DescriptorHeap> samplerDescHeap_ = nullptr;
		mutable std::atomic<UINT> samplerDescFrontIndex_ = 0;
		std::unique_ptr<DeferredRelease> deferredRelease_ = nullptr;

		void EnableFeatures(std::vector<UUID>& features) const;
		void CreateDXDevice(std::wstring gpuVendorName);
		void InitializeStableDevice();
		void InitializeLatestDevice();
		bool CheckWorkGraphSupport();
		bool CheckWorkGraphMeshNodeSupport();
		void CreateDebugDevice(ComPtr<ID3D12DebugDevice>& debugDevice);
		void CreateCommandQueue();
		void CreateDescriptorHeap();
		template<typename T>
		void DeferredReleaser(T* p) const;

	public:
		Device() = default;
		~Device();
		Device(const Device&) = delete;
		Device& operator=(const Device&) = delete;
		void Init(std::wstring gpuVendorName);
		void Init(std::wstring gpuVendorName, ComPtr<ID3D12DebugDevice>& debugDevice);
		void ShowUsedVramSize();
		ASHandle				CreateAS(std::wstring name, UINT64 size) const;
		BLASHandle				CreateBLAS(std::wstring name, const CommandHandle& command, const MeshHandle& mesh) const;
		BufferHandle			CreateBuffer(std::wstring name, BufferType type, UINT strideSize, UINT numElement, D3D12_RESOURCE_FLAGS resourceFlag = D3D12_RESOURCE_FLAG_NONE) const;
		CommandHandle			CreateCommand(std::wstring name, D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT, UINT frameCount = 1) const;
		ComputePipelineHandle	CreateComputePipeline(std::wstring name, const ComputeDesc& desc) const;
		DescriptorManagerHandle	CreateDescriptorManager(std::initializer_list<DescriptorManagerDesc> descManagerDesc, DescriptorLayout descLayout) const;
		FenceHandle 			CreateFence(QueueType queueType, std::wstring name = L"") const;
		GraphicsPipelineHandle	CreateGraphicsPipeline(std::wstring name, const GraphicsDesc& desc) const;
		GuiHandle				CreateGui(const HWND& hwnd) const;
		IndirectHandle			CreateIndirect(std::wstring name, std::initializer_list<IndirectDesc> indirectDescs, UINT byteStride, const RootSignatureHandle& rootSignature = nullptr) const;
		MeshHandle				CreateMesh(std::wstring name, const CommandHandle& command, std::string modelPath) const;
		MeshHandle				CreateMesh(std::wstring name, const CommandHandle& command, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) const;
		MeshPipelineHandle		CreateMeshPipeline(std::wstring name, const MeshDesc& desc) const;
		RaytracingHandle		CreateRaytracing(std::wstring name, const StateObjectHandle& stateObject, std::vector<std::vector<LocalRootArguments>> rayGenLocalRootArguments, std::vector<std::vector<LocalRootArguments>> missLocalRootArguments, std::vector<std::vector<LocalRootArguments>> hitGroupLocalRootArguments) const;
		RootSignatureHandle		CreateRootSignature(std::wstring name, D3D12_ROOT_SIGNATURE_FLAGS flag, std::initializer_list<RootParameter> rootParams) const;
		StateObjectHandle		CreateStateObject(std::wstring name, const StateObjectDesc soDesc) const;
		SwapChainHandle			CreateSwapChain(std::wstring name, const HWND& hwnd, SIZE winSize, UINT bufferCount = 2) const;
		TextureHandle			CreateTexture(std::wstring name, TextureDim texDim, TextureType type, UINT strideSize, DXGI_FORMAT format, UINT width, UINT height, UINT depth, UINT mipLevels = 1) const;
		TLASHandle				CreateTLAS(std::wstring name, const CommandHandle& command, const std::vector<TLASDesc>& tlasDescs) const;
		WorkGraphHandle			CreateWorkGraph(std::wstring name, const StateObjectHandle& stateObject, std::vector<std::vector<LocalRootArguments>> computeLocalRootArguments, std::vector<std::vector<LocalRootArguments>> graphicsLocalRootArguments, UINT maxInputRecords = 0, UINT maxNodeInputs = 0) const;

		void ReleaseCompleted() const;
		void WaitOnCPU(const TimelinePoint& timelinePoint) const;
		UINT AllocateResourceDescriptorFrontIndex(UINT count) const;
		UINT AllocateSamplerDescriptorFrontIndex(UINT count) const;

		const ComPtr<IDXGIFactory7>& GetDXGIFactory() const;
		const ComPtr<ID3D12Device>& GetDevice() const;
		const ComPtr<StableDevice>& GetStableDevice() const;
		const ComPtr<LatestDevice>& GetLatestDevice() const;
		const ComPtr<ID3D12CommandQueue>& GetCommandQueue(QueueType queueType) const;
		FenceHandle GetFence(QueueType queueType) const;
		const ComPtr<ID3D12DescriptorHeap>& GetResourceDescriptorHeap() const;
		const ComPtr<ID3D12DescriptorHeap>& GetSamplerDescriptorHeap() const;
	};
}