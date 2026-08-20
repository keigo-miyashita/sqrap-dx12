#include "Pipeline.hpp"

#include "Device.hpp"
#include "Rootsignature.hpp"
#include "Shader.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	GraphicsDesc::GraphicsDesc(vector<D3D12_INPUT_ELEMENT_DESC> inputLayouts) : inputLayouts_(inputLayouts)
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
			throw runtime_error("Failed to CreateGraphicsPipelineState : " + to_string(result));
		}
		pipeline_->SetName(name_.c_str());
	}

	GraphicsPipeline::GraphicsPipeline(const Device& device, wstring name, const GraphicsDesc& desc)
		: pDevice_(&device), desc_(desc), name_(name)
	{
		CreateGraphicsPipelineState();
	}

	ComPtr<ID3D12PipelineState> GraphicsPipeline::GetPipelineState() const
	{
		return pipeline_;
	}

	MeshPipeline::MeshPipeline(const Device& device, wstring name, const MeshDesc& desc)
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
			throw runtime_error("Failed to CreateMeshPipelineState : " + to_string(result));
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
			throw runtime_error("Failed to CreateComputePipelineState : " + to_string(result));
		}
		pipeline_->SetName(name_.c_str());
	}

	ComputePipeline::ComputePipeline(const Device& device, wstring name, const ComputeDesc& desc)
		: pDevice_(&device), desc_(desc), name_(name)
	{
		CreateComputePipelineState();
	}

	ComPtr<ID3D12PipelineState> ComputePipeline::GetPipelineState() const
	{
		return pipeline_;
	}

	StateObject::StateObject(const Device& device, wstring name, const StateObjectDesc soDesc)
		: pDevice_(&device), soDesc_(soDesc), name_(name)
	{
		if (soDesc.stateObjectType_ == StateObjectType::Raytracing) {
			stateObjectDesc_.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);
			auto pSoConfig = stateObjectDesc_.CreateSubobject<CD3DX12_STATE_OBJECT_CONFIG_SUBOBJECT>();
			if (holds_alternative<StateObjectDesc::RayTracingDesc>(soDesc_.typeDesc_)) {
				StateObjectDesc::RayTracingDesc rtDesc = get<StateObjectDesc::RayTracingDesc>(soDesc_.typeDesc_);

				if (rtDesc.globalRootSig_) {
					auto pGlobalRootSig = stateObjectDesc_.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
					pGlobalRootSig->SetRootSignature(rtDesc.globalRootSig_->GetRootSignature().Get());
				}

				for (auto raygen : rtDesc.rayGens_) {
					auto pLib = stateObjectDesc_.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
					CD3DX12_SHADER_BYTECODE bcLib(raygen.shader_->GetBlob()->GetBufferPointer(), raygen.shader_->GetBlob()->GetBufferSize());
					pLib->SetDXILLibrary(&bcLib);
					// NOTE : �G���g������w�肵�ăR���p�C�����Ă���͂�����SetDXILLibrary�������ƃt�@�C����̃G�N�X�|�[�g���S���o�^����Ă��܂�
					// ���񖾎����Ȃ��Ɠo�^���d�����Ă��܂�
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
					misses.push_back(miss.shader_->GetEntryName().c_str());

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
				// Graphics node��g���Ƃ��͉��̃R�����g�A�E�g��O��.
				pSoConfig->SetFlags(D3D12_STATE_OBJECT_FLAG_WORK_GRAPHS_USE_GRAPHICS_STATE_FOR_GLOBAL_ROOT_SIGNATURE);
			}

			if (holds_alternative<StateObjectDesc::WorkGraphDesc>(soDesc_.typeDesc_)) {
				StateObjectDesc::WorkGraphDesc wgDesc = get<StateObjectDesc::WorkGraphDesc>(soDesc_.typeDesc_);

				if (wgDesc.globalRootSig_) {
					auto pGlobalRootSig = stateObjectDesc_.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
					pGlobalRootSig->SetRootSignature(wgDesc.globalRootSig_->GetRootSignature().Get());
				}

				for (auto exportDesc : wgDesc.exportDescs_) {
					auto pLib = stateObjectDesc_.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
					CD3DX12_SHADER_BYTECODE bcLib(exportDesc.shader_->GetBlob()->GetBufferPointer(), exportDesc.shader_->GetBlob()->GetBufferSize());
					pLib->SetDXILLibrary(&bcLib);
					pLib->DefineExport(exportDesc.shader_->GetEntryName().c_str());

					if (exportDesc.localResourceSet_) {
						auto pLocalRootSig = stateObjectDesc_.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
						pLocalRootSig->SetRootSignature(exportDesc.localResourceSet_->GetRootSignature()->GetRootSignature().Get());

						auto association = stateObjectDesc_.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
						association->SetSubobjectToAssociate(*pLocalRootSig);
						association->AddExport(exportDesc.shader_->GetEntryName().c_str());
					}
				}

				if (soDesc.stateObjectType_ == StateObjectType::WorkGraphMesh) {
					// RTFormat は全プログラムで共通。topology / rasterizer / blend /
					// depthStencil はここで既定値を作り、ProgramDesc 側に指定があれば上書きする
					auto pPrimitiveTopology = stateObjectDesc_.CreateSubobject<CD3DX12_PRIMITIVE_TOPOLOGY_SUBOBJECT>();
					pPrimitiveTopology->SetPrimitiveTopologyType(wgDesc.topologyType_);

					auto pRTFormats = stateObjectDesc_.CreateSubobject<CD3DX12_RENDER_TARGET_FORMATS_SUBOBJECT>();
					pRTFormats->SetNumRenderTargets(static_cast<UINT>(wgDesc.RTVFormats_.size()));
					for (UINT i = 0; i < wgDesc.RTVFormats_.size(); i++) {
						pRTFormats->SetRenderTargetFormat(i, wgDesc.RTVFormats_[i]);
					}

					// 深度バッファの形式。宣言しないと「深度ユニットは DSV を要求しているのに
					// 未宣言」という警告 (STATE_CREATION WARNING #1414) が出る
					auto pDSFormat = stateObjectDesc_.CreateSubobject<CD3DX12_DEPTH_STENCIL_FORMAT_SUBOBJECT>();
					pDSFormat->SetDepthStencilFormat(wgDesc.dsvFormat_);

					// D3D12_RASTERIZER_DESC の内容をサブオブジェクトへ写す
					auto setRasterizer = [](CD3DX12_RASTERIZER_SUBOBJECT* p, const D3D12_RASTERIZER_DESC& d) {
						p->SetFillMode(d.FillMode);
						p->SetCullMode(d.CullMode);
						p->SetFrontCounterClockwise(d.FrontCounterClockwise);
						p->SetDepthBias(static_cast<FLOAT>(d.DepthBias));
						p->SetDepthBiasClamp(d.DepthBiasClamp);
						p->SetSlopeScaledDepthBias(d.SlopeScaledDepthBias);
						p->SetDepthClipEnable(d.DepthClipEnable);
						p->SetForcedSampleCount(d.ForcedSampleCount);
						p->SetConservativeRaster(d.ConservativeRaster);
					};
					auto pRasterizer = stateObjectDesc_.CreateSubobject<CD3DX12_RASTERIZER_SUBOBJECT>();
					setRasterizer(pRasterizer, wgDesc.rasterizerDesc_);

					auto pBlend = stateObjectDesc_.CreateSubobject<CD3DX12_BLEND_SUBOBJECT>();
					pBlend->SetAlphaToCoverageEnable(wgDesc.blendState_.AlphaToCoverageEnable);
					pBlend->SetIndependentBlendEnable(wgDesc.blendState_.IndependentBlendEnable);
					for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++) {
						pBlend->SetRenderTarget(i, wgDesc.blendState_.RenderTarget[i]);
					}

					auto pDepthStencil = stateObjectDesc_.CreateSubobject<CD3DX12_DEPTH_STENCIL_SUBOBJECT>();
					pDepthStencil->SetDepthEnable(wgDesc.depthStencilDesc_.DepthEnable);
					pDepthStencil->SetDepthWriteMask(wgDesc.depthStencilDesc_.DepthWriteMask);
					pDepthStencil->SetDepthFunc(wgDesc.depthStencilDesc_.DepthFunc);
					pDepthStencil->SetStencilEnable(wgDesc.depthStencilDesc_.StencilEnable);
					pDepthStencil->SetStencilReadMask(wgDesc.depthStencilDesc_.StencilReadMask);
					pDepthStencil->SetStencilWriteMask(wgDesc.depthStencilDesc_.StencilWriteMask);
					pDepthStencil->SetFrontFace(wgDesc.depthStencilDesc_.FrontFace);
					pDepthStencil->SetBackFace(wgDesc.depthStencilDesc_.BackFace);

					for (auto programDesc : wgDesc.programDescs_) {
						auto pGenericProgram = stateObjectDesc_.CreateSubobject<CD3DX12_GENERIC_PROGRAM_SUBOBJECT>();
						pGenericProgram->SetProgramName(programDesc.programName_.c_str());
						for (auto shader : programDesc.shaders_) {
							pGenericProgram->AddExport(shader->GetEntryName().c_str());
						}
						if (programDesc.topologyType_.has_value()) {
							auto pProgramTopology = stateObjectDesc_.CreateSubobject<CD3DX12_PRIMITIVE_TOPOLOGY_SUBOBJECT>();
							pProgramTopology->SetPrimitiveTopologyType(*programDesc.topologyType_);
							pGenericProgram->AddSubobject(*pProgramTopology);
						} else {
							pGenericProgram->AddSubobject(*pPrimitiveTopology);
						}

						pGenericProgram->AddSubobject(*pRTFormats);
						pGenericProgram->AddSubobject(*pDSFormat);

						if (programDesc.rasterizerDesc_.has_value()) {
							auto pProgramRasterizer = stateObjectDesc_.CreateSubobject<CD3DX12_RASTERIZER_SUBOBJECT>();
							setRasterizer(pProgramRasterizer, *programDesc.rasterizerDesc_);
							pGenericProgram->AddSubobject(*pProgramRasterizer);
						} else {
							pGenericProgram->AddSubobject(*pRasterizer);
						}

						if (programDesc.blendState_.has_value()) {
							auto pProgramBlend = stateObjectDesc_.CreateSubobject<CD3DX12_BLEND_SUBOBJECT>();
							pProgramBlend->SetAlphaToCoverageEnable(programDesc.blendState_->AlphaToCoverageEnable);
							pProgramBlend->SetIndependentBlendEnable(programDesc.blendState_->IndependentBlendEnable);
							for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++) {
								pProgramBlend->SetRenderTarget(i, programDesc.blendState_->RenderTarget[i]);
							}
							pGenericProgram->AddSubobject(*pProgramBlend);
						} else {
							pGenericProgram->AddSubobject(*pBlend);
						}

						if (programDesc.depthStencilDesc_.has_value()) {
							auto pProgramDepthStencil = stateObjectDesc_.CreateSubobject<CD3DX12_DEPTH_STENCIL_SUBOBJECT>();
							pProgramDepthStencil->SetDepthEnable(programDesc.depthStencilDesc_->DepthEnable);
							pProgramDepthStencil->SetDepthWriteMask(programDesc.depthStencilDesc_->DepthWriteMask);
							pProgramDepthStencil->SetDepthFunc(programDesc.depthStencilDesc_->DepthFunc);
							pProgramDepthStencil->SetStencilEnable(programDesc.depthStencilDesc_->StencilEnable);
							pProgramDepthStencil->SetStencilReadMask(programDesc.depthStencilDesc_->StencilReadMask);
							pProgramDepthStencil->SetStencilWriteMask(programDesc.depthStencilDesc_->StencilWriteMask);
							pProgramDepthStencil->SetFrontFace(programDesc.depthStencilDesc_->FrontFace);
							pProgramDepthStencil->SetBackFace(programDesc.depthStencilDesc_->BackFace);
							pGenericProgram->AddSubobject(*pProgramDepthStencil);
						} else {
							pGenericProgram->AddSubobject(*pDepthStencil);
						}
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
				throw runtime_error("Failed to CreateStateObject : " + to_string(result));
			}
		}
		else if (soDesc.stateObjectType_ == StateObjectType::WorkGraph || soDesc.stateObjectType_ == StateObjectType::WorkGraphMesh) {
			HRESULT result = pDevice_->GetLatestDevice()->CreateStateObject(stateObjectDesc_, IID_PPV_ARGS(stateObject_.ReleaseAndGetAddressOf()));
			if (FAILED(result)) {
				throw runtime_error("Failed to CreateStateObject : " + to_string(result));
			}
		}
	}

	ComPtr<ID3D12StateObject> StateObject::GetStateObject() const
	{
		return stateObject_;
	}

	wstring StateObject::GetProgramName() const
	{
		if (holds_alternative<StateObjectDesc::WorkGraphDesc>(soDesc_.typeDesc_)) {
			StateObjectDesc::WorkGraphDesc wgDesc = get<StateObjectDesc::WorkGraphDesc>(soDesc_.typeDesc_);
			return wgDesc.workGraphProgramName_;
		}
		else {
			throw runtime_error("Cannot get program name for raytracing pipeline !");
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

	vector<wstring> StateObject::GetRayGens() const
	{
		return rayGens;
	}

	vector<wstring> StateObject::GetMisses() const
	{
		return misses;
	}

	vector<wstring> StateObject::GetHitGroups() const
	{
		return hitGroups;
	}
}