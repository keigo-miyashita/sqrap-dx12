#pragma once

#include "pch.hpp"

#include "Alias.hpp"

class Device;
class ResourceSet;
class RootSignature;
class Shader;

struct GraphicsDesc
{
	std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayouts_;
	RootSignatureHandle rootSignature_;
	ShaderHandle VS_;
	ShaderHandle PS_;
	ShaderHandle DS_;
	ShaderHandle HS_;
	ShaderHandle GS_;
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

struct StateObjectDesc
{
	struct ShaderExportDesc
	{
		ShaderHandle shader;
		ShaderStage shaderStage;
		std::shared_ptr<ResourceSet> localResourceSet;
	};

	struct HitGroupExportDesc
	{
		struct HitGroupShaderExportDesc
		{
			ShaderHandle shader;
			ShaderStage shaderStage;
		};

		std::wstring groupName;
		HitGroupShaderExportDesc closesthit;
		HitGroupShaderExportDesc anyhit;
		HitGroupShaderExportDesc intersection;
		std::shared_ptr<ResourceSet> localResourceSet;
	};

	struct RayConfigDesc
	{
		UINT payloadSize;
		UINT attributeSize = 1;
		UINT rayDepth = 1;
	};

	struct RayTracingDesc
	{
		RootSignatureHandle globalRootSig;
		// NOTE : HitGroupExportDescÇ™wstringÇä‹ÇÒÇ≈Ç¢ÇÈÇ©ÇÁÇ©initializer_listÇæÇ∆Ç§Ç‹Ç≠Ç¢Ç©Ç»Ç¢
		std::vector<ShaderExportDesc> rayGens;
		std::vector<ShaderExportDesc> misses;
		std::vector<HitGroupExportDesc> hitGroups;
		RayConfigDesc rayConfigDesc;
	};

	struct ProgramDesc
	{
		std::wstring programName;
		std::initializer_list<ShaderHandle> shaders;
	};

	struct WorkGraphDesc
	{
		RootSignatureHandle globalRootSig;
		std::initializer_list<ShaderExportDesc> exportDescs;
		// NOTE : initializer_listÇÇ¬Ç©Ç§Ç∆ProgramDesc.programNameÇ™ê≥ÇµÇ≠ë„ì¸Ç≥ÇÍÇ»Ç¢
		// Ç®ÇªÇÁÇ≠éıñΩÇÃñ‚ëËÇ≈wstringÇ≈ÇÕÇ§Ç‹Ç≠Ç¢Ç©Ç»Ç¢â¬î\ê´Ç†ÇË
		std::vector<ProgramDesc> programDescs;
		std::wstring workGraphProgramName = L"Program";
	};

	StateObjectType stateObjectType;
	std::variant<RayTracingDesc, WorkGraphDesc> typeDesc;
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