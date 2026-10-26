#pragma once

#include "pch.hpp"

#include "Alias.hpp"

namespace sqrp
{
	class Device;
	class RootSignature;
	class Shader;

	enum class StateObjectType
	{
		Raytracing, WorkGraph
	};

	inline const std::vector<D3D12_INPUT_ELEMENT_DESC> DEFAULT_INPUT_LAYOUT =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	struct RenderState
	{
		D3D12_BLEND_DESC blendState_ = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		UINT sampleMask_ = D3D12_DEFAULT_SAMPLE_MASK;
		D3D12_RASTERIZER_DESC rasterizerDesc_ = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType_ = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		std::vector<DXGI_FORMAT> rtvFormats_ = { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB };   // SwapChain の RTV（sRGB）と揃える
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc_ = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		DXGI_FORMAT dsvFormat_ = DXGI_FORMAT_D32_FLOAT;
		DXGI_SAMPLE_DESC sampleDesc_ = { 1, 0 };
	};

	struct GraphicsDesc
	{
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayouts_;
		RootSignatureHandle rootSignature_;
		ShaderHandle VS_ = nullptr;
		ShaderHandle PS_ = nullptr;
		ShaderHandle DS_ = nullptr;
		ShaderHandle HS_ = nullptr;
		ShaderHandle GS_ = nullptr;
		RenderState renderState_;
		D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue_ = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

		GraphicsDesc(std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayouts = DEFAULT_INPUT_LAYOUT);

		GraphicsDesc& SetRootSignature(const RootSignatureHandle& rootSignature);
		GraphicsDesc& SetVS(const ShaderHandle& VS);
		GraphicsDesc& SetPS(const ShaderHandle& PS);
		GraphicsDesc& SetTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType);
	};

	class GraphicsPipeline
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		std::wstring name_;
		ComPtr<ID3D12PipelineState> pipeline_ = nullptr;

	public:
		GraphicsPipeline(const Device& device, std::wstring name, const GraphicsDesc& desc);
		GraphicsPipeline(const GraphicsPipeline&) = delete;
		GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;
		const ComPtr<ID3D12PipelineState>& GetPipelineState() const;
	};

	struct MeshDesc
	{
		RootSignatureHandle rootSignature_;
		ShaderHandle AS_ = nullptr;
		ShaderHandle MS_ = nullptr;
		ShaderHandle PS_ = nullptr;
		RenderState renderState_;

		MeshDesc() = default;
	};

	class MeshPipeline
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		std::wstring name_;
		ComPtr<ID3D12PipelineState> pipeline_ = nullptr;

	public:
		MeshPipeline(const Device& device, std::wstring name, const MeshDesc& desc);
		MeshPipeline(const MeshPipeline&) = delete;
		MeshPipeline& operator=(const MeshPipeline&) = delete;
		const ComPtr<ID3D12PipelineState>& GetPipelineState() const;
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

		std::wstring name_;
		ComPtr<ID3D12PipelineState> pipeline_ = nullptr;

	public:
		ComputePipeline(const Device& device, std::wstring name, const ComputeDesc& desc);
		ComputePipeline(const ComputePipeline&) = delete;
		ComputePipeline& operator=(const ComputePipeline&) = delete;
		const ComPtr<ID3D12PipelineState>& GetPipelineState() const;
	};

	enum class NodeType
	{
		Compute, Graphics,
	};

	struct StateObjectDesc
	{
		// Shader and its local root signature for Ray tracing(Raygen, Miss, Callable), WorkGraph(Compute node)
		// Only the shape (root signature) is given here. The local root arguments are given to Raytracing / WorkGraph
		struct ShaderExportDesc
		{
			ShaderHandle shader_;
			RootSignatureHandle localRootSignature_;
			std::wstring nodeName_; // compute node name (work graph only)
			UINT nodeIndex_ = 0; // compute node index if it is node array (work graph only)
		};

		// Shaders and the local root signature for HitGroup(ClosestHit, Anyhit, Intersection)
		struct HitGroupExportDesc
		{
			std::wstring groupName_;
			ShaderHandle closestHit_;
			ShaderHandle anyHit_;
			ShaderHandle intersection_;
			RootSignatureHandle localRootSignature_;
		};

		// Ray tracing config
		struct RayConfigDesc
		{
			UINT payloadSize_ = 0;
			UINT attributeSize_ = 1;
			UINT rayDepth_ = 1;
		};

		// StateObject Setting for ray tracing
		struct RaytracingDesc
		{
			RootSignatureHandle globalRootSig_;
			std::vector<ShaderExportDesc> rayGens_;
			std::vector<ShaderExportDesc> misses_;
			std::vector<HitGroupExportDesc> hitGroups_;
			RayConfigDesc rayConfigDesc_;
		};

		// Program setting for work graph
		// 描画の設定（renderState_）はグラフィックスノードのプログラムだけが持つ
		// （WorkGraphDesc には全プログラム共通の設定は無い）
		struct ProgramDesc
		{
			std::wstring programName_; // program name
			std::wstring nodeName_; // mesh node name
			UINT nodeIndex_ = 0; // mesh node index if it is node array
			ShaderHandle meshShader_;
			ShaderHandle pixelShader_;
			RootSignatureHandle localRootSignature_;
			RenderState renderState_;
		};

		// StateObject setting for work graph
		struct WorkGraphDesc
		{
			RootSignatureHandle globalRootSig_;
			// Compute nodes
			std::vector<ShaderExportDesc> computeNodeDescs_;
			// Graphics nodes (one mesh node per program)
			std::vector<ProgramDesc> graphicsProgramDescs_;
			std::wstring workGraphProgramName_ = L"Program";
		};

		std::variant<RaytracingDesc, WorkGraphDesc> typeDesc_;
	};

	class StateObject
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		std::wstring name_;
		ComPtr<ID3D12StateObject> stateObject_ = nullptr;
		StateObjectType soType_ = StateObjectType::Raytracing;
		RootSignatureHandle globalRootSig_ = nullptr;
		std::vector<StateObjectDesc::ShaderExportDesc> rayGens_;
		std::vector<StateObjectDesc::ShaderExportDesc> misses_;
		std::vector<StateObjectDesc::HitGroupExportDesc> hitGroups_;
		std::vector<StateObjectDesc::ShaderExportDesc> computeNodes_;
		std::vector<StateObjectDesc::ProgramDesc> graphicsNodes_;
		std::wstring workGraphProgramName_;
		bool hasGraphicsNodes_ = false;

		void CreateStateObject(const Device& device, const StateObjectDesc::RaytracingDesc& rtDesc);
		void CreateStateObject(const Device& device, const StateObjectDesc::WorkGraphDesc& wgDesc);

	public:
		StateObject(const Device& device, std::wstring name, const StateObjectDesc soDesc);
		StateObject(const StateObject&) = delete;
		StateObject& operator=(const StateObject&) = delete;
		const ComPtr<ID3D12StateObject>& GetStateObject() const;
		StateObjectType GetStateObjectType() const;
		const std::wstring& GetWorkGraphProgramName() const;
		const std::vector<StateObjectDesc::ShaderExportDesc>& GetRayGens() const;
		const std::vector<StateObjectDesc::ShaderExportDesc>& GetMisses() const;
		const std::vector<StateObjectDesc::HitGroupExportDesc>& GetHitGroups() const;
		const std::vector<StateObjectDesc::ShaderExportDesc>& GetComputeNodes() const;
		const std::vector<StateObjectDesc::ProgramDesc>& GetGraphicsNodes() const;
		bool HasGraphicsNodes() const;
	};
}