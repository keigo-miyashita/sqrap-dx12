#include "Pipeline.hpp"

#include "Device.hpp"
#include "Rootsignature.hpp"
#include "Shader.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	GraphicsDesc::GraphicsDesc(std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayouts) : inputLayouts_(inputLayouts)
	{

	}

	GraphicsDesc& GraphicsDesc::SetRootSignature(RootSignatureHandle rootSignature)
	{
		rootSignature_ = rootSignature;
		return *this;
	}

	GraphicsDesc& GraphicsDesc::SetVS(ShaderHandle VS)
	{
		VS_ = VS;
		return *this;
	}

	GraphicsDesc& GraphicsDesc::SetPS(ShaderHandle PS)
	{
		PS_ = PS;
		return *this;
	}

	GraphicsDesc& GraphicsDesc::SetPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveType)
	{
		primitiveType_ = primitiveType;
		return *this;
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

	GraphicsPipeline::GraphicsPipeline(const Device& device, std::wstring name, const GraphicsDesc& desc)
		: pDevice_(&device), desc_(desc), name_(name)
	{
		CreateGraphicsPipelineState();
	}

	ComPtr<ID3D12PipelineState> GraphicsPipeline::GetPipelineState() const
	{
		return pipeline_;
	}

	MeshDesc::MeshDesc()
	{

	}

	MeshPipeline::MeshPipeline(const Device& device, std::wstring name, const MeshDesc& desc)
		: pDevice_(&device), desc_(desc), name_(name)
	{
		D3DX12_MESH_SHADER_PIPELINE_STATE_DESC meshPSDesc = {};

		meshPSDesc.pRootSignature = desc_.rootSignature_->GetRootSignature().Get();
		if (desc_.AS_) {
			meshPSDesc.AS = CD3DX12_SHADER_BYTECODE(desc_.AS_->GetBlob()->GetBufferPointer(), desc_.AS_->GetBlob()->GetBufferSize());
		}
		if (desc_.MS_) {
			meshPSDesc.MS = CD3DX12_SHADER_BYTECODE(desc_.MS_->GetBlob()->GetBufferPointer(), desc_.MS_->GetBlob()->GetBufferSize());
		}
		if (desc_.PS_) {
			meshPSDesc.PS = CD3DX12_SHADER_BYTECODE(desc_.PS_->GetBlob()->GetBufferPointer(), desc_.PS_->GetBlob()->GetBufferSize());
		}
		meshPSDesc.BlendState = desc_.blendState_;
		meshPSDesc.SampleMask = desc_.sampleMask_;
		meshPSDesc.RasterizerState = desc_.rasterizerDesc_;
		meshPSDesc.DepthStencilState = desc_.depthStencilDesc_;
		meshPSDesc.DSVFormat = desc_.dsvFormat_;
		meshPSDesc.PrimitiveTopologyType = desc_.primitiveType_;
		meshPSDesc.NumRenderTargets = desc_.RTVFormats_.size();
		for (int i = 0; i < desc_.RTVFormats_.size(); i++) {
			meshPSDesc.RTVFormats[i] = desc_.RTVFormats_[i];
		}
		meshPSDesc.SampleDesc = desc_.sampleDesc_;

		CD3DX12_PIPELINE_MESH_STATE_STREAM meshStateStream = CD3DX12_PIPELINE_MESH_STATE_STREAM(meshPSDesc);

		D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
		streamDesc.pPipelineStateSubobjectStream = &meshStateStream;
		streamDesc.SizeInBytes = sizeof(meshStateStream);

		auto result = pDevice_->GetStableDevice()->CreatePipelineState(&streamDesc, IID_PPV_ARGS(pipeline_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw std::runtime_error("Failed to CreateMeshPipelineState : " + to_string(result));
		}
		pipeline_->SetName(name_.c_str());
	}

	ComPtr<ID3D12PipelineState> MeshPipeline::GetPipelineState() const
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

	ComputePipeline::ComputePipeline(const Device& device, std::wstring name, const ComputeDesc& desc)
		: pDevice_(&device), desc_(desc), name_(name)
	{
		CreateComputePipelineState();
	}

	ComPtr<ID3D12PipelineState> ComputePipeline::GetPipelineState() const
	{
		return pipeline_;
	}

	StateObject::StateObject(const Device& device, std::wstring name, const StateObjectDesc soDesc)
		: pDevice_(&device), soDesc_(soDesc), name_(name)
	{
		if (soDesc.stateObjectType_ == StateObjectType::Raytracing) {
			stateObjectDesc_.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);
			auto pSoConfig = stateObjectDesc_.CreateSubobject<CD3DX12_STATE_OBJECT_CONFIG_SUBOBJECT>();
			if (std::holds_alternative<StateObjectDesc::RayTracingDesc>(soDesc_.typeDesc_)) {
				StateObjectDesc::RayTracingDesc rtDesc = std::get<StateObjectDesc::RayTracingDesc>(soDesc_.typeDesc_);

				if (rtDesc.globalRootSig_) {
					auto pGlobalRootSig = stateObjectDesc_.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
					pGlobalRootSig->SetRootSignature(rtDesc.globalRootSig_->GetRootSignature().Get());
				}

				for (auto raygen : rtDesc.rayGens_) {
					auto pLib = stateObjectDesc_.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
					CD3DX12_SHADER_BYTECODE bcLib(raygen.shader_->GetBlob()->GetBufferPointer(), raygen.shader_->GetBlob()->GetBufferSize());
					pLib->SetDXILLibrary(&bcLib);
					// NOTE : エントリ名を指定してコンパイルしているはずだがSetDXILLibraryだけだとファイル内のエクスポートが全部登録されてしまう
					// 毎回明示しないと登録が重複してしまう
					pLib->DefineExport(raygen.shader_->GetEntryName().c_str());
					rayGens.push_back(raygen.shader_->GetEntryName().c_str());

					if (raygen.localResourceSet_) {
						auto pLocalRootSig = stateObjectDesc_.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
						pLocalRootSig->SetRootSignature(raygen.localResourceSet_->GetRootSignature()->GetRootSignature().Get());

						auto association = stateObjectDesc_.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
						association->SetSubobjectToAssociate(*pLocalRootSig);
						association->AddExport(raygen.shader_->GetEntryName().c_str());
					}
				}

				for (auto miss : rtDesc.misses_) {
					auto pLib = stateObjectDesc_.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
					CD3DX12_SHADER_BYTECODE bcLib(miss.shader_->GetBlob()->GetBufferPointer(), miss.shader_->GetBlob()->GetBufferSize());
					pLib->SetDXILLibrary(&bcLib);
					pLib->DefineExport(miss.shader_->GetEntryName().c_str());
					rayGens.push_back(miss.shader_->GetEntryName().c_str());

					if (miss.localResourceSet_) {
						auto pLocalRootSig = stateObjectDesc_.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
						pLocalRootSig->SetRootSignature(miss.localResourceSet_->GetRootSignature()->GetRootSignature().Get());

						auto association = stateObjectDesc_.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
						association->SetSubobjectToAssociate(*pLocalRootSig);
						association->AddExport(miss.shader_->GetEntryName().c_str());
					}
				}

				for (auto hitGroup : rtDesc.hitGroups_) {
					auto pLib = stateObjectDesc_.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
					CD3DX12_SHADER_BYTECODE closesthitBcLib(hitGroup.closesthit_.shader_->GetBlob()->GetBufferPointer(), hitGroup.closesthit_.shader_->GetBlob()->GetBufferSize());
					pLib->SetDXILLibrary(&closesthitBcLib);
					pLib->DefineExport(hitGroup.closesthit_.shader_->GetEntryName().c_str());
					if (hitGroup.anyhit_.shader_) {
						CD3DX12_SHADER_BYTECODE anyhitBcLib(hitGroup.anyhit_.shader_->GetBlob()->GetBufferPointer(), hitGroup.anyhit_.shader_->GetBlob()->GetBufferSize());
						pLib->SetDXILLibrary(&anyhitBcLib);
						pLib->DefineExport(hitGroup.anyhit_.shader_->GetEntryName().c_str());
					}
					if (hitGroup.intersection_.shader_) {
						CD3DX12_SHADER_BYTECODE intersectionBcLib(hitGroup.intersection_.shader_->GetBlob()->GetBufferPointer(), hitGroup.intersection_.shader_->GetBlob()->GetBufferSize());
						pLib->SetDXILLibrary(&intersectionBcLib);
						pLib->DefineExport(hitGroup.intersection_.shader_->GetEntryName().c_str());
					}

					auto pHitGroup = stateObjectDesc_.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
					pHitGroup->SetClosestHitShaderImport(hitGroup.closesthit_.shader_->GetEntryName().c_str());
					if (hitGroup.anyhit_.shader_) {
						pHitGroup->SetAnyHitShaderImport(hitGroup.anyhit_.shader_->GetEntryName().c_str());
					}
					if (hitGroup.intersection_.shader_) {
						pHitGroup->SetIntersectionShaderImport(hitGroup.intersection_.shader_->GetEntryName().c_str());
					}
					pHitGroup->SetHitGroupExport(hitGroup.groupName_.c_str());

					hitGroups.push_back(hitGroup.groupName_.c_str());

					if (hitGroup.localResourceSet_) {
						auto pLocalRootSig = stateObjectDesc_.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
						pLocalRootSig->SetRootSignature(hitGroup.localResourceSet_->GetRootSignature()->GetRootSignature().Get());

						auto association = stateObjectDesc_.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
						association->SetSubobjectToAssociate(*pLocalRootSig);
						association->AddExport(hitGroup.groupName_.c_str());
					}
				}

				auto pShaderConfig = stateObjectDesc_.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
				// payload size, setting for intersection
				pShaderConfig->Config(rtDesc.rayConfigDesc_.payloadSize_, rtDesc.rayConfigDesc_.attributeSize_);

				auto pPipelineConfig = stateObjectDesc_.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
				// ray depth
				pPipelineConfig->Config(rtDesc.rayConfigDesc_.rayDepth_);
			}
		}
		else if (soDesc.stateObjectType_ == StateObjectType::WorkGraph || soDesc.stateObjectType_ == StateObjectType::WorkGraphMesh) {
			stateObjectDesc_.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_EXECUTABLE);
			auto pSoConfig = stateObjectDesc_.CreateSubobject<CD3DX12_STATE_OBJECT_CONFIG_SUBOBJECT>();
			if (soDesc.stateObjectType_ == StateObjectType::WorkGraphMesh) {
				// Graphics nodeを使うときは下のコメントアウトを外す.
				pSoConfig->SetFlags(D3D12_STATE_OBJECT_FLAG_WORK_GRAPHS_USE_GRAPHICS_STATE_FOR_GLOBAL_ROOT_SIGNATURE);
			}

			if (std::holds_alternative<StateObjectDesc::WorkGraphDesc>(soDesc_.typeDesc_)) {
				StateObjectDesc::WorkGraphDesc wgDesc = std::get<StateObjectDesc::WorkGraphDesc>(soDesc_.typeDesc_);

				if (wgDesc.globalRootSig_) {
					auto pGlobalRootSig = stateObjectDesc_.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
					pGlobalRootSig->SetRootSignature(wgDesc.globalRootSig_->GetRootSignature().Get());
				}

				for (auto exportDesc : wgDesc.exportDescs_) {
					auto pLib = stateObjectDesc_.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
					CD3DX12_SHADER_BYTECODE bcLib(exportDesc.shader_->GetBlob()->GetBufferPointer(), exportDesc.shader_->GetBlob()->GetBufferSize());
					pLib->SetDXILLibrary(&bcLib);

					if (exportDesc.localResourceSet_) {
						auto pLocalRootSig = stateObjectDesc_.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
						pLocalRootSig->SetRootSignature(exportDesc.localResourceSet_->GetRootSignature()->GetRootSignature().Get());

						auto association = stateObjectDesc_.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
						association->SetSubobjectToAssociate(*pLocalRootSig);
						association->AddExport(exportDesc.shader_->GetEntryName().c_str());
					}
				}

				if (soDesc.stateObjectType_ == StateObjectType::WorkGraphMesh) {
					// NOTE : 
					// This wrapper is designed under the assumption
					// All program use the same topology and RTFormat
					auto pPrimitiveTopology = stateObjectDesc_.CreateSubobject<CD3DX12_PRIMITIVE_TOPOLOGY_SUBOBJECT>();
					pPrimitiveTopology->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
					auto pRTFormats = stateObjectDesc_.CreateSubobject<CD3DX12_RENDER_TARGET_FORMATS_SUBOBJECT>();
					pRTFormats->SetNumRenderTargets(1);
					pRTFormats->SetRenderTargetFormat(0, DXGI_FORMAT_R8G8B8A8_UNORM);

					for (auto programDesc : wgDesc.programDescs_) {
						auto pGenericProgram = stateObjectDesc_.CreateSubobject<CD3DX12_GENERIC_PROGRAM_SUBOBJECT>();
						pGenericProgram->SetProgramName(programDesc.programName_.c_str());
						for (auto shader : programDesc.shaders_) {
							pGenericProgram->AddExport(shader->GetEntryName().c_str());
						}
						pGenericProgram->AddSubobject(*pPrimitiveTopology);
						pGenericProgram->AddSubobject(*pRTFormats);
					}
				}

				auto pWorkGraph = stateObjectDesc_.CreateSubobject<CD3DX12_WORK_GRAPH_SUBOBJECT>();
				pWorkGraph->IncludeAllAvailableNodes();
				pWorkGraph->SetProgramName(wgDesc.workGraphProgramName_.c_str());

				if (soDesc.stateObjectType_ == StateObjectType::WorkGraphMesh) {
					for (auto programDesc : wgDesc.programDescs_) {
						if (programDesc.nodeType_ == NodeType::Compute) {
							auto pComputeNode = pWorkGraph->CreateShaderNode(programDesc.programName_.c_str());
						}
						else if (programDesc.nodeType_ == NodeType::Graphics) {
							auto pGraphicsNode = pWorkGraph->CreateProgramNode(programDesc.programName_.c_str());
						}
					}
				}
			}
		}

		if (soDesc.stateObjectType_ == StateObjectType::Raytracing) {
			HRESULT result = pDevice_->GetStableDevice()->CreateStateObject(stateObjectDesc_, IID_PPV_ARGS(stateObject_.ReleaseAndGetAddressOf()));
			if (FAILED(result)) {
				throw std::runtime_error("Failed to CreateStateObject : " + to_string(result));
			}
		}
		else if (soDesc.stateObjectType_ == StateObjectType::WorkGraph || soDesc.stateObjectType_ == StateObjectType::WorkGraphMesh) {
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
		if (std::holds_alternative<StateObjectDesc::WorkGraphDesc>(soDesc_.typeDesc_)) {
			StateObjectDesc::WorkGraphDesc wgDesc = std::get<StateObjectDesc::WorkGraphDesc>(soDesc_.typeDesc_);
			return wgDesc.workGraphProgramName_;
		}
		else {
			throw std::runtime_error("Cannot get program name for raytracing pipeline !");
		}
	}

	StateObjectType StateObject::GetStateObjectType() const
	{
		return soDesc_.stateObjectType_;
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
}