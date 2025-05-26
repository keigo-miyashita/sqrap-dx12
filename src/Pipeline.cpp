#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

GraphicsDesc::GraphicsDesc(std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayouts) : inputLayouts_(inputLayouts)
{

}

bool GraphicsPipeline::CreateGraphicsPipelineState(const GraphicsDesc& desc, wstring name)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPSDesc = {};

	graphicsPSDesc.InputLayout.pInputElementDescs = desc.inputLayouts_.data();
	graphicsPSDesc.InputLayout.NumElements = desc.inputLayouts_.size();
	graphicsPSDesc.pRootSignature = desc.rootSignature_->GetRootSignature().Get();
	if (desc.VS_) {
		graphicsPSDesc.VS = CD3DX12_SHADER_BYTECODE(desc.VS_->GetBlob()->GetBufferPointer(), desc.VS_->GetBlob()->GetBufferSize());
	}
	if (desc.PS_) {
		graphicsPSDesc.PS = CD3DX12_SHADER_BYTECODE(desc.PS_->GetBlob()->GetBufferPointer(), desc.PS_->GetBlob()->GetBufferSize());
	}
	if (desc.DS_) {
		graphicsPSDesc.DS = CD3DX12_SHADER_BYTECODE(desc.DS_->GetBlob()->GetBufferPointer(), desc.DS_->GetBlob()->GetBufferSize());
	}
	if (desc.HS_) {
		graphicsPSDesc.HS = CD3DX12_SHADER_BYTECODE(desc.HS_->GetBlob()->GetBufferPointer(), desc.HS_->GetBlob()->GetBufferSize());
	}
	if (desc.GS_) {
		graphicsPSDesc.GS = CD3DX12_SHADER_BYTECODE(desc.GS_->GetBlob()->GetBufferPointer(), desc.GS_->GetBlob()->GetBufferSize());
	}
	graphicsPSDesc.BlendState = desc.blendState_;
	graphicsPSDesc.SampleMask = desc.sampleMask_;
	graphicsPSDesc.RasterizerState = desc.rasterizerDesc_;
	graphicsPSDesc.DepthStencilState = desc.depthStencilDesc_;
	graphicsPSDesc.DSVFormat = desc.dsvFormat_;
	graphicsPSDesc.IBStripCutValue = desc.IBStripCutValue_;
	graphicsPSDesc.PrimitiveTopologyType = desc.primitiveType_;
	graphicsPSDesc.NumRenderTargets = desc.RTVFormats_.size();
	for (int i = 0; i < desc.RTVFormats_.size(); i++) {
		graphicsPSDesc.RTVFormats[i] = desc.RTVFormats_[i];
	}
	graphicsPSDesc.SampleDesc = desc.sampleDesc_;

	auto result = pDevice_->GetDevice()->CreateGraphicsPipelineState(&graphicsPSDesc, IID_PPV_ARGS(pipeline_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		cerr << "Failed to create graphics pipeline state" << endl;
		return false;
	}
	pipeline_->SetName(name.c_str());
	
	return true;
}

GraphicsPipeline::GraphicsPipeline()
{

}

bool GraphicsPipeline::Init(Device* pDevice, const GraphicsDesc& desc, wstring name)
{
	pDevice_ = pDevice;
	if (pDevice_ == nullptr) {
		cerr << "GraphicsPipeline class doesn't have any pointer" << endl;
		return false;
	}
	if (!CreateGraphicsPipelineState(desc, name)) {
		return false;
	}

	return true;
}

ComPtr<ID3D12PipelineState> GraphicsPipeline::GetPipelineState() const
{
	return pipeline_;
}

bool ComputePipeline::CreateComputePipelineState(const ComputeDesc& desc, wstring name)
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePSDesc = {};
	computePSDesc.pRootSignature = desc.rootSignature_->GetRootSignature().Get();
	computePSDesc.CS = CD3DX12_SHADER_BYTECODE(desc.CS_->GetBlob()->GetBufferPointer(), desc.CS_->GetBlob()->GetBufferSize());
	computePSDesc.NodeMask = desc.nodeMask_;

	if (FAILED(pDevice_->GetDevice()->CreateComputePipelineState(&computePSDesc, IID_PPV_ARGS(pipeline_.ReleaseAndGetAddressOf())))) {
		return false;
	}
	pipeline_->SetName(name.c_str());

	return true;
}

ComputePipeline::ComputePipeline()
{

}

bool ComputePipeline::Init(Device* pDevice, const ComputeDesc& desc, wstring name)
{
	pDevice_ = pDevice;
	if (pDevice_ == nullptr) {
		cerr << "ComputePipeline class doesn't have any pointer" << endl;
		return false;
	}
	if (!CreateComputePipelineState(desc, name)) {
		return false;
	}

	return true;
}

ComPtr<ID3D12PipelineState> ComputePipeline::GetPipelineState() const
{
	return pipeline_;
}

void StateObjectDesc::AddGlobalRootSignature(const RootSignature& rootSignature)
{
	auto pGlobalRootSig = stateObjectDesc_.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	pGlobalRootSig->SetRootSignature(rootSignature.GetRootSignature().Get());
}

void StateObjectDesc::AddShader(const Shader& shader)
{
	auto pLib = stateObjectDesc_.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	CD3DX12_SHADER_BYTECODE bcLib(shader.GetBlob()->GetBufferPointer(), shader.GetBlob()->GetBufferSize());
	pLib->SetDXILLibrary(&bcLib);
}

void StateObjectDesc::AddWorkgraph(wstring programName)
{
	auto pWorkGraph = stateObjectDesc_.CreateSubobject<CD3DX12_WORK_GRAPH_SUBOBJECT>();
	pWorkGraph->IncludeAllAvailableNodes();
	pWorkGraph->SetProgramName(programName.c_str());
	programName_ = programName;
}

void StateObjectDesc::AddGenericProgram(std::vector<std::wstring> entries)
{
	auto pPrimitiveTopology = stateObjectDesc_.CreateSubobject<CD3DX12_PRIMITIVE_TOPOLOGY_SUBOBJECT>();
	pPrimitiveTopology->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	auto pRTFormats = stateObjectDesc_.CreateSubobject<CD3DX12_RENDER_TARGET_FORMATS_SUBOBJECT>();
	pRTFormats->SetNumRenderTargets(1);
	pRTFormats->SetRenderTargetFormat(0, DXGI_FORMAT_R8G8B8A8_UNORM);

	auto pGenericProgram = stateObjectDesc_.CreateSubobject<CD3DX12_GENERIC_PROGRAM_SUBOBJECT>();
	pGenericProgram->SetProgramName(L"program");
	for (int i = 0; i < entries.size(); i++) {
		pGenericProgram->AddExport(entries[i].c_str());
	}
	pGenericProgram->AddSubobject(*pPrimitiveTopology);
	pGenericProgram->AddSubobject(*pRTFormats);
}

StateObjectDesc::StateObjectDesc()
{


}

void StateObjectDesc::Init(StateObjectType::Type type)
{
	stateObjectType_ = type;
	if (stateObjectType_ == StateObjectType::Raytracing) {
		stateObjectDesc_.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);
		auto pSoConfig = stateObjectDesc_.CreateSubobject<CD3DX12_STATE_OBJECT_CONFIG_SUBOBJECT>();
	}
	else if (stateObjectType_ == StateObjectType::WorkGraph || stateObjectType_ == StateObjectType::WorkGraphMesh) {
		stateObjectDesc_.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_EXECUTABLE);
		auto pSoConfig = stateObjectDesc_.CreateSubobject<CD3DX12_STATE_OBJECT_CONFIG_SUBOBJECT>();
		// Graphics nodeを使うときは下のコメントアウトを外す.
		pSoConfig->SetFlags(D3D12_STATE_OBJECT_FLAG_WORK_GRAPHS_USE_GRAPHICS_STATE_FOR_GLOBAL_ROOT_SIGNATURE);
	}
}

CD3DX12_STATE_OBJECT_DESC& StateObjectDesc::GetStateObjectDesc()
{
	return stateObjectDesc_;
}

StateObjectType::Type StateObjectDesc::GetStateObjectType() const
{
	return stateObjectType_;
}

std::wstring StateObjectDesc::GetProgramName() const
{
	return programName_;
}

bool StateObject::CreateStateObject(StateObjectDesc& soDesc, wstring name)
{
	cout << "StateObject::CreateStateObject" << endl;
	StateObjectType::Type test = soDesc.GetStateObjectType();
	stateObjectType_ = soDesc.GetStateObjectType();
	if (soDesc.GetStateObjectType() == StateObjectType::Raytracing) {
		cout << "StateObjectType::Raytracing" << endl;
	}
	else if (soDesc.GetStateObjectType() == StateObjectType::WorkGraph || soDesc.GetStateObjectType() == StateObjectType::WorkGraphMesh) {
		cout << "StateObjectType::WorkGraph" << endl;
		if (FAILED(pDevice_->GetLatestDevice()->CreateStateObject(soDesc.GetStateObjectDesc(), IID_PPV_ARGS(stateObject_.ReleaseAndGetAddressOf())))) {
			return false;
		}
		return true;
	}
}

StateObject::StateObject()
{

}

bool StateObject::Init(Device* pDevice, StateObjectDesc& soDesc, wstring name)
{
	cout << "StateObject::Init" << endl;
	pDevice_ = pDevice;
	if (pDevice_ == nullptr) {
		cerr << "StateObject class doesn't have any pointer" << endl;
		return false;
	}
	programName_ = soDesc.GetProgramName();
	if (!CreateStateObject(soDesc, name))
	{
		return false;
	}

	return true;
}

ComPtr<ID3D12StateObject> StateObject::GetStateObject() const
{
	return stateObject_;
}

std::wstring StateObject::GetProgramName() const
{
	return programName_;
}

StateObjectType::Type StateObject::GetStateObjectType() const
{
	return stateObjectType_;
}