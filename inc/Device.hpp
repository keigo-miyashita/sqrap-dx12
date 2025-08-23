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
		ASHandle				CreateAS(UINT size, std::wstring name = L"") const;
		ASMeshHandle			CreateASMesh(CommandHandle command, std::string modelPath) const;
		ASMeshHandle			CreateASMesh(CommandHandle command, const std::vector<ASVertex>& ASVertices, const std::vector<uint32_t>& indices) const;
		BLASHandle				CreateBLAS(CommandHandle command, ASMeshHandle mesh, std::wstring name = L"") const;
		BufferHandle			CreateBuffer(BufferType type, UINT strideSize, UINT numElement, std::wstring name = L"") const;
		CommandHandle			CreateCommand(D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT, std::wstring name = L"") const;
		ComputePipelineHandle	CreateComputePipeline(const ComputeDesc& desc, std::wstring name = L"") const;
		DescriptorManagerHandle	CreateDescriptorManager(HeapType heapType, std::initializer_list<DescriptorManagerDesc> descManagerDesc, D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE, std::wstring name = L"") const;
		FenceHandle				CreateFence(std::wstring name = L"") const;
		GraphicsPipelineHandle	CreateGraphicsPipeline(const GraphicsDesc& desc, std::wstring name = L"") const;
		GUIHandle				CreateGUI(const HWND& hwnd) const;
		IndirectHandle			CreateIndirect(std::initializer_list<IndirectDesc> indirectDescs, RootSignatureHandle rootSignature, UINT byteStride, UINT maxCommandCount = 0, BufferHandle argumentBuffer = nullptr, BufferHandle counterBuffer = nullptr, std::wstring name = L"") const;
		MeshHandle				CreateMesh(CommandHandle command, std::string modelPath) const;
		MeshHandle				CreateMesh(CommandHandle command, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) const;
		MeshPipelineHandle		CreateMeshPipeline(const MeshDesc& desc, std::wstring name = L"") const;
		RayTracingHandle		CreateRaytracing(StateObjectHandle stateObject, UINT width, UINT height, UINT depth, std::wstring name = L"") const;
		RootSignatureHandle		CreateRootSignature(D3D12_ROOT_SIGNATURE_FLAGS flag, std::initializer_list<RootParameter> rootParams, std::wstring name = L"") const;
		StateObjectHandle		CreateStateObject(const StateObjectDesc soDesc, std::wstring name = L"") const;
		SwapChainHandle			CreateSwapChain(CommandHandle command, const HWND& hwnd, SIZE winSize, std::wstring name = L"") const;
		TextureHandle			CreateTexture(TextureDim texDim, TextureType type, UINT strideSize, DXGI_FORMAT format, UINT width, UINT height, UINT depth, std::wstring name = L"");
		TLASHandle				CreateTLAS(CommandHandle command, const std::vector<TLASDesc>& tlasDescs, std::wstring name = L"") const;
		WorkGraphHandle			CreateWorkGraph(StateObjectHandle stateObject, UINT maxInputRecords = 0, UINT maxNodeInputs = 0, std::wstring name = L"") const;

		ComPtr<IDXGIFactory7> GetDXGIFactory() const;
		ComPtr<ID3D12Device> GetDevice() const;
		ComPtr<StableDevice> GetStableDevice() const;
		ComPtr<LatestDevice> GetLatestDevice() const;
		ComPtr<ID3D12CommandQueue> GetGraphicsCommandQueue() const;
		ComPtr<ID3D12CommandQueue> GetComputeCommandQueue() const;
	};
}