#include "Pipeline.hpp"

#include "Device.hpp"
#include "Rootsignature.hpp"
#include "Shader.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

GraphicsDesc::GraphicsDesc(std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayouts) : inputLayouts_(inputLayouts)
{

}

void GraphicsPipeline::CreateGraphicsPipelineState()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPSDesc = {};

	graphicsPSDesc.InputLayout.pInputElementDescs = desc_.inputLayouts_.data();
	graphicsPSDesc.InputLayout.NumElements = desc_.inputLayouts_.size();
	graphicsPSDesc.pRootSignature = desc_.rootSignature_->GetRootSignature().Get();
	if (desc_.VS_) {
		graphicsPSDesc.VS = CD3DX12_SHADER_BYTECODE(desc_.VS_->GetBlob()->GetBufferPointer(), desc_.VS_->GetBlob()->GetBufferSize());
	}
	if (desc_.PS_) {
		graphicsPSDesc.PS = CD3DX12_SHADER_BYTECODE(desc_.PS_->GetBlob()->GetBufferPointer(), desc_.PS_->GetBlob()->GetBufferSize());
	}
	if (desc_.DS_) {
		graphicsPSDesc.DS = CD3DX12_SHADER_BYTECODE(desc_.DS_->GetBlob()->GetBufferPointer(), desc_.DS_->GetBlob()->GetBufferSize());
	}
	if (desc_.HS_) {
		graphicsPSDesc.HS = CD3DX12_SHADER_BYTECODE(desc_.HS_->GetBlob()->GetBufferPointer(), desc_.HS_->GetBlob()->GetBufferSize());
	}
	if (desc_.GS_) {
		graphicsPSDesc.GS = CD3DX12_SHADER_BYTECODE(desc_.GS_->GetBlob()->GetBufferPointer(), desc_.GS_->GetBlob()->GetBufferSize());
	}
	graphicsPSDesc.BlendState = desc_.blendState_;
	graphicsPSDesc.SampleMask = desc_.sampleMask_;
	graphicsPSDesc.RasterizerState = desc_.rasterizerDesc_;
	graphicsPSDesc.DepthStencilState = desc_.depthStencilDesc_;
	graphicsPSDesc.DSVFormat = desc_.dsvFormat_;
	graphicsPSDesc.IBStripCutValue = desc_.IBStripCutValue_;
	graphicsPSDesc.PrimitiveTopologyType = desc_.primitiveType_;
	graphicsPSDesc.NumRenderTargets = desc_.RTVFormats_.size();
	for (int i = 0; i < desc_.RTVFormats_.size(); i++) {
		graphicsPSDesc.RTVFormats[i] = desc_.RTVFormats_[i];
	}
	graphicsPSDesc.SampleDesc = desc_.sampleDesc_;

	auto result = pDevice_->GetDevice()->CreateGraphicsPipelineState(&graphicsPSDesc, IID_PPV_ARGS(pipeline_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		throw std::runtime_error("Failed to CreateGraphicsPipelineState : " + to_string(result));
	}
	pipeline_->SetName(name_.c_str());
}

GraphicsPipeline::GraphicsPipeline(const Device& device, const GraphicsDesc& desc, std::wstring name)
	: pDevice_(&device), desc_(desc), name_(name)
{
	CreateGraphicsPipelineState();
}

ComPtr<ID3D12PipelineState> GraphicsPipeline::GetPipelineState() const
{
	return pipeline_;
}

void ComputePipeline::CreateComputePipelineState()
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePSDesc = {};
	computePSDesc.pRootSignature = desc_.rootSignature_->GetRootSignature().Get();
	computePSDesc.CS = CD3DX12_SHADER_BYTECODE(desc_.CS_->GetBlob()->GetBufferPointer(), desc_.CS_->GetBlob()->GetBufferSize());
	computePSDesc.NodeMask = desc_.nodeMask_;

	HRESULT result = pDevice_->GetDevice()->CreateComputePipelineState(&computePSDesc, IID_PPV_ARGS(pipeline_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		throw std::runtime_error("Failed to CreateComputePipelineState : " + to_string(result));
	}
	pipeline_->SetName(name_.c_str());
}

ComputePipeline::ComputePipeline(const Device& device, const ComputeDesc& desc, std::wstring name)
	: pDevice_(&device), desc_(desc), name_(name)
{
	CreateComputePipelineState();
}

ComPtr<ID3D12PipelineState> ComputePipeline::GetPipelineState() const
{
	return pipeline_;
}

StateObject::StateObject(const Device& device, const StateObjectDesc soDesc, std::wstring name)
	: pDevice_(&device), soDesc_(soDesc), name_(name)
{
	if (soDesc.stateObjectType == StateObjectType::Raytracing) {
		stateObjectDesc_.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);
		auto pSoConfig = stateObjectDesc_.CreateSubobject<CD3DX12_STATE_OBJECT_CONFIG_SUBOBJECT>();
		if (std::holds_alternative<StateObjectDesc::RayTracingDesc>(soDesc_.typeDesc)) {
			StateObjectDesc::RayTracingDesc rtDesc = std::get<StateObjectDesc::RayTracingDesc>(soDesc_.typeDesc);
			
			if (rtDesc.globalRootSig) {
				auto pGlobalRootSig = stateObjectDesc_.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
				pGlobalRootSig->SetRootSignature(rtDesc.globalRootSig->GetRootSignature().Get());
			}

			for (auto raygen : rtDesc.rayGens) {
				auto pLib = stateObjectDesc_.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
				CD3DX12_SHADER_BYTECODE bcLib(raygen.shader->GetBlob()->GetBufferPointer(), raygen.shader->GetBlob()->GetBufferSize());
				pLib->SetDXILLibrary(&bcLib);
				// NOTE : エントリ名を指定してコンパイルしているはずだがSetDXILLibraryだけだとファイル内のエクスポートが全部登録されてしまう
				// 毎回明示しないと登録が重複しちゃった
				pLib->DefineExport(raygen.shader->GetEntryName().c_str());
				rayGens.push_back(raygen.shader->GetEntryName().c_str());

				if (raygen.localResourceSet) {
					auto pLocalRootSig = stateObjectDesc_.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
					pLocalRootSig->SetRootSignature(raygen.localResourceSet->GetRootSignature()->GetRootSignature().Get());

					auto association = stateObjectDesc_.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
					association->SetSubobjectToAssociate(*pLocalRootSig);
					association->AddExport(raygen.shader->GetEntryName().c_str());
				}
			}

			for (auto miss : rtDesc.misses) {
				auto pLib = stateObjectDesc_.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
				CD3DX12_SHADER_BYTECODE bcLib(miss.shader->GetBlob()->GetBufferPointer(), miss.shader->GetBlob()->GetBufferSize());
				pLib->SetDXILLibrary(&bcLib);
				pLib->DefineExport(miss.shader->GetEntryName().c_str());
				rayGens.push_back(miss.shader->GetEntryName().c_str());

				if (miss.localResourceSet) {
					auto pLocalRootSig = stateObjectDesc_.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
					pLocalRootSig->SetRootSignature(miss.localResourceSet->GetRootSignature()->GetRootSignature().Get());

					auto association = stateObjectDesc_.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
					association->SetSubobjectToAssociate(*pLocalRootSig);
					association->AddExport(miss.shader->GetEntryName().c_str());
				}
			}

			for (auto hitGroup : rtDesc.hitGroups) {
				auto pLib = stateObjectDesc_.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
				CD3DX12_SHADER_BYTECODE closesthitBcLib(hitGroup.closesthit.shader->GetBlob()->GetBufferPointer(),		hitGroup.closesthit.shader->GetBlob()->GetBufferSize());
				pLib->SetDXILLibrary(&closesthitBcLib);
				pLib->DefineExport(hitGroup.closesthit.shader->GetEntryName().c_str());
				if (hitGroup.anyhit.shader) {
					CD3DX12_SHADER_BYTECODE anyhitBcLib(hitGroup.anyhit.shader->GetBlob()->GetBufferPointer(), hitGroup.anyhit.shader->GetBlob()->GetBufferSize());
					pLib->SetDXILLibrary(&anyhitBcLib);
					pLib->DefineExport(hitGroup.anyhit.shader->GetEntryName().c_str());
				}
				if (hitGroup.intersection.shader) {
					CD3DX12_SHADER_BYTECODE intersectionBcLib(hitGroup.intersection.shader->GetBlob()->GetBufferPointer(), hitGroup.intersection.shader->GetBlob()->GetBufferSize());
					pLib->SetDXILLibrary(&intersectionBcLib);
					pLib->DefineExport(hitGroup.intersection.shader->GetEntryName().c_str());
				}

				auto pHitGroup = stateObjectDesc_.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
				pHitGroup->SetClosestHitShaderImport(hitGroup.closesthit.shader->GetEntryName().c_str());
				if (hitGroup.anyhit.shader) {
					pHitGroup->SetAnyHitShaderImport(hitGroup.anyhit.shader->GetEntryName().c_str());
				}
				if (hitGroup.intersection.shader) {
					pHitGroup->SetIntersectionShaderImport(hitGroup.intersection.shader->GetEntryName().c_str());
				}
				pHitGroup->SetHitGroupExport(hitGroup.groupName.c_str());

				hitGroups.push_back(hitGroup.groupName.c_str());

				if (hitGroup.localResourceSet) {
					auto pLocalRootSig = stateObjectDesc_.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
					pLocalRootSig->SetRootSignature(hitGroup.localResourceSet->GetRootSignature()->GetRootSignature().Get());

					auto association = stateObjectDesc_.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
					association->SetSubobjectToAssociate(*pLocalRootSig);
					association->AddExport(hitGroup.groupName.c_str());
				}
			}

			auto pShaderConfig = stateObjectDesc_.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
			// payload size, setting for intersection
			pShaderConfig->Config(rtDesc.rayConfigDesc.payloadSize, rtDesc.rayConfigDesc.attributeSize);

			auto pPipelineConfig = stateObjectDesc_.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
			// ray depth
			pPipelineConfig->Config(rtDesc.rayConfigDesc.rayDepth);
		}
	}
	else if (soDesc.stateObjectType == StateObjectType::WorkGraph || soDesc.stateObjectType == StateObjectType::WorkGraphMesh) {
		stateObjectDesc_.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_EXECUTABLE);
		auto pSoConfig = stateObjectDesc_.CreateSubobject<CD3DX12_STATE_OBJECT_CONFIG_SUBOBJECT>();
		if (soDesc.stateObjectType == StateObjectType::WorkGraphMesh) {
			// Graphics nodeを使うときは下のコメントアウトを外す.
			pSoConfig->SetFlags(D3D12_STATE_OBJECT_FLAG_WORK_GRAPHS_USE_GRAPHICS_STATE_FOR_GLOBAL_ROOT_SIGNATURE);
		}

		if (std::holds_alternative<StateObjectDesc::WorkGraphDesc>(soDesc_.typeDesc)) {
			StateObjectDesc::WorkGraphDesc wgDesc = std::get<StateObjectDesc::WorkGraphDesc>(soDesc_.typeDesc);

			if (wgDesc.globalRootSig) {
				auto pGlobalRootSig = stateObjectDesc_.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
				pGlobalRootSig->SetRootSignature(wgDesc.globalRootSig->GetRootSignature().Get());
			}

			for (auto exportDesc : wgDesc.exportDescs) {
				auto pLib = stateObjectDesc_.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
				CD3DX12_SHADER_BYTECODE bcLib(exportDesc.shader->GetBlob()->GetBufferPointer(), exportDesc.shader->GetBlob()->GetBufferSize());
				pLib->SetDXILLibrary(&bcLib);

				if (exportDesc.localResourceSet) {
					auto pLocalRootSig = stateObjectDesc_.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
					pLocalRootSig->SetRootSignature(exportDesc.localResourceSet->GetRootSignature()->GetRootSignature().Get());

					auto association = stateObjectDesc_.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
					association->SetSubobjectToAssociate(*pLocalRootSig);
					association->AddExport(exportDesc.shader->GetEntryName().c_str());
				}
			}

			if (soDesc.stateObjectType == StateObjectType::WorkGraphMesh) {
				// NOTE : 
				// This wrapper is designed under the assumption
				// All program use the same topology and RTFormat
				auto pPrimitiveTopology = stateObjectDesc_.CreateSubobject<CD3DX12_PRIMITIVE_TOPOLOGY_SUBOBJECT>();
				pPrimitiveTopology->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
				auto pRTFormats = stateObjectDesc_.CreateSubobject<CD3DX12_RENDER_TARGET_FORMATS_SUBOBJECT>();
				pRTFormats->SetNumRenderTargets(1);
				pRTFormats->SetRenderTargetFormat(0, DXGI_FORMAT_R8G8B8A8_UNORM);

				for (auto programDesc : wgDesc.programDescs) {
					auto pGenericProgram = stateObjectDesc_.CreateSubobject<CD3DX12_GENERIC_PROGRAM_SUBOBJECT>();
					pGenericProgram->SetProgramName(programDesc.programName.c_str());
					for (auto shader : programDesc.shaders) {
						pGenericProgram->AddExport(shader->GetEntryName().c_str());
					}
					pGenericProgram->AddSubobject(*pPrimitiveTopology);
					pGenericProgram->AddSubobject(*pRTFormats);
				}
			}

			auto pWorkGraph = stateObjectDesc_.CreateSubobject<CD3DX12_WORK_GRAPH_SUBOBJECT>();
			pWorkGraph->IncludeAllAvailableNodes();
			pWorkGraph->SetProgramName(wgDesc.workGraphProgramName.c_str());
		}
	}

	if (soDesc.stateObjectType == StateObjectType::Raytracing) {
		// TODO : Handle raytracing pipeline
		cout << "StateObjectType::Raytracing" << endl;
		HRESULT result = pDevice_->GetStableDevice()->CreateStateObject(stateObjectDesc_, IID_PPV_ARGS(stateObject_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw std::runtime_error("Failed to CreateStateObject : " + to_string(result));
		}
	}
	else if (soDesc.stateObjectType == StateObjectType::WorkGraph || soDesc.stateObjectType == StateObjectType::WorkGraphMesh) {
		cout << "StateObjectType::WorkGraph" << endl;
		HRESULT result = pDevice_->GetLatestDevice()->CreateStateObject(stateObjectDesc_, IID_PPV_ARGS(stateObject_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw std::runtime_error("Failed to CreateStateObject : " + to_string(result));
		}
	}
}

ComPtr<ID3D12StateObject> StateObject::GetStateObject() const
{
	return stateObject_;
}

std::wstring StateObject::GetProgramName() const
{
	if (std::holds_alternative<StateObjectDesc::WorkGraphDesc>(soDesc_.typeDesc)) {
		StateObjectDesc::WorkGraphDesc wgDesc = std::get<StateObjectDesc::WorkGraphDesc>(soDesc_.typeDesc);
		return wgDesc.workGraphProgramName;
	}
	// TODO : Throw exception
}

StateObjectType StateObject::GetStateObjectType() const
{
	return soDesc_.stateObjectType;
}

StateObjectDesc StateObject::GetStateObjectDesc() const
{
	return soDesc_;
}

std::vector<std::wstring> StateObject::GetRayGens() const
{
	return rayGens;
}

std::vector<std::wstring> StateObject::GetMisses() const
{
	return misses;
}

std::vector<std::wstring> StateObject::GetHitGroups() const
{
	return hitGroups;
}