#pragma once

#include <common.hpp>

class Device;
class RootSignature;
class Shader;

struct GraphicsDesc
{
	std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayouts_;
	std::shared_ptr<RootSignature> rootSignature_;
	std::shared_ptr<Shader> VS_;
	std::shared_ptr<Shader> PS_;
	std::shared_ptr<Shader> DS_;
	std::shared_ptr<Shader> HS_;
	std::shared_ptr<Shader> GS_;
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

	bool CreateGraphicsPipelineState();

public:
	GraphicsPipeline(const Device& device, const GraphicsDesc& desc, std::wstring name = L"");
	~GraphicsPipeline() = default;
	ComPtr<ID3D12PipelineState> GetPipelineState() const;
};

struct ComputeDesc
{
	std::shared_ptr<RootSignature> rootSignature_;
	std::shared_ptr<Shader> CS_;
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

	bool CreateComputePipelineState();

public:
	ComputePipeline(const Device& device, const ComputeDesc& desc, std::wstring name = L"");
	~ComputePipeline() = default;
	ComPtr<ID3D12PipelineState> GetPipelineState() const;
};

struct StateObjectType
{
	enum Type {
		Raytracing,
		WorkGraph,
		WorkGraphMesh
	};
};

class StateObjectDesc
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	CD3DX12_STATE_OBJECT_DESC stateObjectDesc_;
	StateObjectType::Type stateObjectType_;
	std::wstring programName_;

public:
	StateObjectDesc();
	~StateObjectDesc() = default;
	void Init(StateObjectType::Type type);
	void AddGlobalRootSignature(const RootSignature& rootSignature);
	void AddShader(const Shader& shader);
	void AddWorkgraph(std::wstring programName);
	void AddGenericProgram(std::vector<std::wstring> entries);
	CD3DX12_STATE_OBJECT_DESC& GetStateObjectDesc();
	StateObjectType::Type GetStateObjectType() const;
	std::wstring GetProgramName() const;
};

class StateObject
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	Device* pDevice_ = nullptr;
	ComPtr<ID3D12StateObject> stateObject_ = nullptr;
	std::wstring programName_;
	StateObjectType::Type stateObjectType_;

	bool CreateStateObject(StateObjectDesc& soDesc, std::wstring name = L"StateObject");

public:
	StateObject();
	~StateObject() = default;
	bool Init(Device* pDevice, StateObjectDesc& soDesc, std::wstring name = L"StateObject");
	ComPtr<ID3D12StateObject> GetStateObject() const;
	std::wstring GetProgramName() const;
	StateObjectType::Type GetStateObjectType() const;
};