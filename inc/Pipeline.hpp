#pragma once

#include "pch.hpp"

#include "Alias.hpp"

namespace sqrp
{
	class Device;
	class ResourceSet;
	class RootSignature;
	class Shader;

	struct GraphicsDesc
	{
		std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayouts_;
		RootSignatureHandle rootSignature_;
		ShaderHandle VS_ = nullptr;
		ShaderHandle PS_ = nullptr;
		ShaderHandle DS_ = nullptr;
		ShaderHandle HS_ = nullptr;
		ShaderHandle GS_ = nullptr;
		D3D12_BLEND_DESC blendState_ = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		UINT sampleMask_ = D3D12_DEFAULT_SAMPLE_MASK;
		D3D12_RASTERIZER_DESC rasterizerDesc_ = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc_ = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		DXGI_FORMAT dsvFormat_ = DXGI_FORMAT_D32_FLOAT;
		D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue_ = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveType_ = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		std::vector<DXGI_FORMAT> RTVFormats_ = { DXGI_FORMAT_R8G8B8A8_UNORM };
		DXGI_SAMPLE_DESC sampleDesc_ = { 1, 0 };

		GraphicsDesc(std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayouts);
	};

	class GraphicsPipeline
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const Device* pDevice_ = nullptr;
		const GraphicsDesc& desc_;
		std::wstring name_;
		ComPtr<ID3D12PipelineState> pipeline_ = nullptr;

		void CreateGraphicsPipelineState();

	public:
		GraphicsPipeline(const Device& device, const GraphicsDesc& desc, std::wstring name = L"");
		~GraphicsPipeline() = default;
		ComPtr<ID3D12PipelineState> GetPipelineState() const;
	};

	struct MeshDesc
	{
		RootSignatureHandle rootSignature_;
		ShaderHandle AS_ = nullptr;
		ShaderHandle MS_ = nullptr;
		ShaderHandle PS_ = nullptr;
		D3D12_BLEND_DESC blendState_ = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		UINT sampleMask_ = D3D12_DEFAULT_SAMPLE_MASK;
		D3D12_RASTERIZER_DESC rasterizerDesc_ = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc_ = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		DXGI_FORMAT dsvFormat_ = DXGI_FORMAT_D32_FLOAT;
		D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveType_ = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		std::vector<DXGI_FORMAT> RTVFormats_ = { DXGI_FORMAT_R8G8B8A8_UNORM };
		DXGI_SAMPLE_DESC sampleDesc_ = { 1, 0 };

		MeshDesc();
	};

	class MeshPipeline
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const Device* pDevice_ = nullptr;
		const MeshDesc& desc_;
		std::wstring name_;
		ComPtr<ID3D12PipelineState> pipeline_ = nullptr;

	public:
		MeshPipeline(const Device& device, const MeshDesc& desc, std::wstring name = L"");
		~MeshPipeline() = default;
		ComPtr<ID3D12PipelineState> GetPipelineState() const;
	};

	struct ComputeDesc
	{
		RootSignatureHandle rootSignature_;
		ShaderHandle CS_;
		UINT nodeMask_ = 0;
	};

	class ComputePipeline
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const Device* pDevice_ = nullptr;
		const ComputeDesc& desc_;
		std::wstring name_;
		ComPtr<ID3D12PipelineState> pipeline_ = nullptr;

		void CreateComputePipelineState();

	public:
		ComputePipeline(const Device& device, const ComputeDesc& desc, std::wstring name = L"");
		~ComputePipeline() = default;
		ComPtr<ID3D12PipelineState> GetPipelineState() const;
	};

	enum class StateObjectType
	{
		Raytracing, WorkGraph, WorkGraphMesh
	};

	enum class ShaderStage
	{
		RayGen, ClosestHit, Anyhit, Intersection, Miss, Callable, Compute, Mesh, Pixel
	};

	enum class NodeType
	{
		Compute, Graphics,
	};

	struct StateObjectDesc
	{
		// Shader and binded resource for Ray tracing(Raygen, Miss, Callable), WorkGraph(Compute, Mesh, Pixel)
		struct ShaderExportDesc
		{
			ShaderHandle shader_;
			ShaderStage shaderStage_;
			ResourceSetHandle localResourceSet_;
		};

		// Shader and binded resource for HitGroup(ClosestHit, Anyhit, Intersection)
		struct HitGroupExportDesc
		{
			struct HitGroupShaderExportDesc
			{
				ShaderHandle shader_;
				ShaderStage shaderStage_;
			};

			std::wstring groupName_;
			HitGroupShaderExportDesc closesthit_;
			HitGroupShaderExportDesc anyhit_;
			HitGroupShaderExportDesc intersection_;
			ResourceSetHandle localResourceSet_;
		};

		// Ray tracing config
		struct RayConfigDesc
		{
			UINT payloadSize_;
			UINT attributeSize_ = 1;
			UINT rayDepth_ = 1;
		};

		// StateObject Setting for ray tracing
		struct RayTracingDesc
		{
			RootSignatureHandle globalRootSig_;
			std::vector<ShaderExportDesc> rayGens_;
			std::vector<ShaderExportDesc> misses_;
			std::vector<HitGroupExportDesc> hitGroups_;
			RayConfigDesc rayConfigDesc_;
		};

		// Program setting for work graph
		struct ProgramDesc
		{
			NodeType nodeType_;
			std::wstring programName_;
			std::vector<ShaderHandle> shaders_;
			ResourceSetHandle resourceSet_;
		};

		// StateObject setting for work graph
		struct WorkGraphDesc
		{
			RootSignatureHandle globalRootSig_;
			std::vector<ShaderExportDesc> exportDescs_;
			std::vector<ProgramDesc> programDescs_;
			std::wstring workGraphProgramName_ = L"Program";
		};

		StateObjectType stateObjectType_;
		std::variant<RayTracingDesc, WorkGraphDesc> typeDesc_;
	};

	class StateObject
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const Device* pDevice_ = nullptr;
		const StateObjectDesc soDesc_;
		std::wstring name_;
		CD3DX12_STATE_OBJECT_DESC stateObjectDesc_;
		ComPtr<ID3D12StateObject> stateObject_ = nullptr;
		std::vector<std::wstring> rayGens;
		std::vector<std::wstring> misses;
		std::vector<std::wstring> hitGroups;
		std::wstring programName_;
		StateObjectType stateObjectType_;

	public:
		StateObject(const Device& device, const StateObjectDesc soDesc, std::wstring name = L"");
		~StateObject() = default;
		ComPtr<ID3D12StateObject> GetStateObject() const;
		std::wstring GetProgramName() const;
		StateObjectType GetStateObjectType() const;
		StateObjectDesc GetStateObjectDesc() const;
		std::vector<std::wstring> GetRayGens() const;
		std::vector<std::wstring> GetMisses() const;
		std::vector<std::wstring> GetHitGroups() const;
	};
}