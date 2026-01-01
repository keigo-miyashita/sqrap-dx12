#pragma once

#include "pch.hpp"

#include "AS.hpp"
#include "Descriptormanager.hpp"
#include "Indirect.hpp"
#include "Mesh.hpp"
#include "Pipeline.hpp"
#include "Raytracing.hpp"
#include "Resource.hpp"
#include "Rootsignature.hpp"
#include "Workgraph.hpp"

#include "Alias.hpp"

using StableDevice = ID3D12Device13;
using LatestDevice = ID3D12Device14;

void DebugOutputFormatString(const char* format, ...);

namespace sqrp
{
	class Command;
	class Fence;
	class GUI;
	class Shader;
	class SwapChain;

	enum class QueueType {
		Graphics, Compute
	};

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

		void EnableFeatures(std::vector<UUID>& features) const;
		void CreateDXDevice(std::wstring gpuVendorName);
		void InitializeStableDevice();
		void InitializeLatestDevice();
		bool CheckWorkGraphSupport();
		void CreateDebugDevice(ComPtr<ID3D12DebugDevice>& debugDevice);
		void CreateCommandQueue();

	public:
		Device();
		~Device() = default;
		void Init(std::wstring gpuVenorName);
		void Init(std::wstring gpuVenorName, ComPtr<ID3D12DebugDevice>& debugDevice);
		void ShowUsedVramSize();
		ASHandle				CreateAS(std::wstring name, UINT size) const;
		ASMeshHandle			CreateASMesh(CommandHandle command, std::string modelPath) const;
		ASMeshHandle			CreateASMesh(std::wstring name, CommandHandle command, const std::vector<ASVertex>& ASVertices, const std::vector<uint32_t>& indices) const;
		BLASHandle				CreateBLAS(std::wstring name, CommandHandle command, ASMeshHandle mesh) const;
		BufferHandle			CreateBuffer(std::wstring name, BufferType type, UINT strideSize, UINT numElement, D3D12_RESOURCE_FLAGS resourceFlag = D3D12_RESOURCE_FLAG_NONE) const;
		CommandHandle			CreateCommand(std::wstring name, D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT) const;
		ComputePipelineHandle	CreateComputePipeline(std::wstring name, const ComputeDesc& desc) const;
		DescriptorManagerHandle	CreateDescriptorManager(std::wstring name, HeapType heapType, std::initializer_list<DescriptorManagerDesc> descManagerDesc, D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE) const;
		FenceHandle				CreateFence(std::wstring name = L"") const;
		GraphicsPipelineHandle	CreateGraphicsPipeline(std::wstring name, const GraphicsDesc& desc) const;
		GUIHandle				CreateGUI(const HWND& hwnd) const;
		IndirectHandle			CreateIndirect(std::wstring name, std::initializer_list<IndirectDesc> indirectDescs, RootSignatureHandle rootSignature, UINT byteStride, UINT maxCommandCount = 0, BufferHandle argumentBuffer = nullptr, BufferHandle counterBuffer = nullptr) const;
		MeshHandle				CreateMesh(CommandHandle command, std::string modelPath) const;
		MeshHandle				CreateMesh(std::wstring name, CommandHandle command, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) const;
		MeshHandle				CreateMesh(std::wstring name, CommandHandle command, UINT verticesNum, UINT indicesNum) const;
		MeshPipelineHandle		CreateMeshPipeline(std::wstring name, const MeshDesc& desc) const;
		RayTracingHandle		CreateRaytracing(std::wstring name, StateObjectHandle stateObject, UINT width, UINT height, UINT depth) const;
		RootSignatureHandle		CreateRootSignature(std::wstring name, D3D12_ROOT_SIGNATURE_FLAGS flag, std::initializer_list<RootParameter> rootParams) const;
		StateObjectHandle		CreateStateObject(std::wstring name, const StateObjectDesc soDesc) const;
		SwapChainHandle			CreateSwapChain(std::wstring name, CommandHandle command, const HWND& hwnd, SIZE winSize) const;
		TextureHandle			CreateTexture(std::wstring name, TextureDim texDim, TextureType type, UINT strideSize, DXGI_FORMAT format, UINT width, UINT height, UINT depth);
		TLASHandle				CreateTLAS(std::wstring name, CommandHandle command, const std::vector<TLASDesc>& tlasDescs) const;
		WorkGraphHandle			CreateWorkGraph(std::wstring name, StateObjectHandle stateObject, UINT maxInputRecords = 0, UINT maxNodeInputs = 0) const;

		ComPtr<IDXGIFactory7> GetDXGIFactory() const;
		ComPtr<ID3D12Device> GetDevice() const;
		ComPtr<StableDevice> GetStableDevice() const;
		ComPtr<LatestDevice> GetLatestDevice() const;
		ComPtr<ID3D12CommandQueue> GetGraphicsCommandQueue() const;
		ComPtr<ID3D12CommandQueue> GetComputeCommandQueue() const;
	};
}