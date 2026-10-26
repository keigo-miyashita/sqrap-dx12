#include "Pipeline.hpp"

#include "Device.hpp"
#include "RootSignature.hpp"
#include "Shader.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	GraphicsDesc::GraphicsDesc(vector<D3D12_INPUT_ELEMENT_DESC> inputLayouts) : inputLayouts_(inputLayouts)
	{

	}

	GraphicsDesc& GraphicsDesc::SetRootSignature(const RootSignatureHandle& rootSignature)
	{
		rootSignature_ = rootSignature;
		return *this;
	}

	GraphicsDesc& GraphicsDesc::SetVS(const ShaderHandle& VS)
	{
		VS_ = VS;
		return *this;
	}

	GraphicsDesc& GraphicsDesc::SetPS(const ShaderHandle& PS)
	{
		PS_ = PS;
		return *this;
	}

	GraphicsDesc& GraphicsDesc::SetTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
	{
		renderState_.topologyType_ = topologyType;
		return *this;
	}

	GraphicsPipeline::GraphicsPipeline(const Device& device, wstring name, const GraphicsDesc& desc)
		: name_(name)
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
		graphicsPSDesc.BlendState = desc.renderState_.blendState_;
		graphicsPSDesc.SampleMask = desc.renderState_.sampleMask_;
		graphicsPSDesc.RasterizerState = desc.renderState_.rasterizerDesc_;
		graphicsPSDesc.DepthStencilState = desc.renderState_.depthStencilDesc_;
		graphicsPSDesc.DSVFormat = desc.renderState_.dsvFormat_;
		graphicsPSDesc.IBStripCutValue = desc.IBStripCutValue_;
		graphicsPSDesc.PrimitiveTopologyType = desc.renderState_.topologyType_;
		if (desc.renderState_.rtvFormats_.size() > D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT) {
			throw runtime_error("[GraphicsPipeline::GraphicsPipeline] rtvFormats_ has more than 8 formats");
		}
		graphicsPSDesc.NumRenderTargets = desc.renderState_.rtvFormats_.size();
		for (int i = 0; i < desc.renderState_.rtvFormats_.size(); i++) {
			graphicsPSDesc.RTVFormats[i] = desc.renderState_.rtvFormats_[i];
		}
		graphicsPSDesc.SampleDesc = desc.renderState_.sampleDesc_;

		auto result = device.GetDevice()->CreateGraphicsPipelineState(&graphicsPSDesc, IID_PPV_ARGS(pipeline_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[GraphicsPipeline::GraphicsPipeline] CreateGraphicsPipelineState failed\nHRESULT = " + to_string(result));
		}
		pipeline_->SetName(name_.c_str());
	}

	const ComPtr<ID3D12PipelineState>& GraphicsPipeline::GetPipelineState() const
	{
		return pipeline_;
	}

	MeshPipeline::MeshPipeline(const Device& device, wstring name, const MeshDesc& desc)
		: name_(name)
	{
		D3DX12_MESH_SHADER_PIPELINE_STATE_DESC meshPSDesc = {};

		meshPSDesc.pRootSignature = desc.rootSignature_->GetRootSignature().Get();
		if (desc.AS_) {
			meshPSDesc.AS = CD3DX12_SHADER_BYTECODE(desc.AS_->GetBlob()->GetBufferPointer(), desc.AS_->GetBlob()->GetBufferSize());
		}
		if (desc.MS_) {
			meshPSDesc.MS = CD3DX12_SHADER_BYTECODE(desc.MS_->GetBlob()->GetBufferPointer(), desc.MS_->GetBlob()->GetBufferSize());
		}
		if (desc.PS_) {
			meshPSDesc.PS = CD3DX12_SHADER_BYTECODE(desc.PS_->GetBlob()->GetBufferPointer(), desc.PS_->GetBlob()->GetBufferSize());
		}
		meshPSDesc.BlendState = desc.renderState_.blendState_;
		meshPSDesc.SampleMask = desc.renderState_.sampleMask_;
		meshPSDesc.RasterizerState = desc.renderState_.rasterizerDesc_;
		meshPSDesc.DepthStencilState = desc.renderState_.depthStencilDesc_;
		meshPSDesc.DSVFormat = desc.renderState_.dsvFormat_;
		meshPSDesc.PrimitiveTopologyType = desc.renderState_.topologyType_;
		if (desc.renderState_.rtvFormats_.size() > D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT) {
			throw runtime_error("[MeshPipeline::MeshPipeline] rtvFormats_ has more than 8 formats");
		}
		meshPSDesc.NumRenderTargets = desc.renderState_.rtvFormats_.size();
		for (int i = 0; i < desc.renderState_.rtvFormats_.size(); i++) {
			meshPSDesc.RTVFormats[i] = desc.renderState_.rtvFormats_[i];
		}
		meshPSDesc.SampleDesc = desc.renderState_.sampleDesc_;

		CD3DX12_PIPELINE_MESH_STATE_STREAM meshStateStream = CD3DX12_PIPELINE_MESH_STATE_STREAM(meshPSDesc);

		D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
		streamDesc.pPipelineStateSubobjectStream = &meshStateStream;
		streamDesc.SizeInBytes = sizeof(meshStateStream);

		auto result = device.GetStableDevice()->CreatePipelineState(&streamDesc, IID_PPV_ARGS(pipeline_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[MeshPipeline::MeshPipeline] CreatePipelineState failed\nHRESULT = " + to_string(result));
		}
		pipeline_->SetName(name_.c_str());
	}

	const ComPtr<ID3D12PipelineState>& MeshPipeline::GetPipelineState() const
	{
		return pipeline_;
	}

	ComputePipeline::ComputePipeline(const Device& device, wstring name, const ComputeDesc& desc)
		: name_(name)
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC computePSDesc = {};
		computePSDesc.pRootSignature = desc.rootSignature_->GetRootSignature().Get();
		computePSDesc.CS = CD3DX12_SHADER_BYTECODE(desc.CS_->GetBlob()->GetBufferPointer(), desc.CS_->GetBlob()->GetBufferSize());
		computePSDesc.NodeMask = desc.nodeMask_;

		HRESULT result = device.GetDevice()->CreateComputePipelineState(&computePSDesc, IID_PPV_ARGS(pipeline_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[ComputePipeline::ComputePipeline] CreateComputePipelineState failed\nHRESULT = " + to_string(result));
		}
		pipeline_->SetName(name_.c_str());
	}

	const ComPtr<ID3D12PipelineState>& ComputePipeline::GetPipelineState() const
	{
		return pipeline_;
	}

	void StateObject::CreateStateObject(const Device& device, const StateObjectDesc::RaytracingDesc& rtDesc)
	{
		soType_ = StateObjectType::Raytracing;

		CD3DX12_STATE_OBJECT_DESC stateObjectDesc;

		globalRootSig_ = rtDesc.globalRootSig_;
		stateObjectDesc.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);
		auto pSoConfig = stateObjectDesc.CreateSubobject<CD3DX12_STATE_OBJECT_CONFIG_SUBOBJECT>();
		if (rtDesc.globalRootSig_) {
			auto pGlobalRootSig = stateObjectDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
			pGlobalRootSig->SetRootSignature(rtDesc.globalRootSig_->GetRootSignature().Get());
		}

		for (auto raygen : rtDesc.rayGens_) {
			auto pLib = stateObjectDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
			CD3DX12_SHADER_BYTECODE bcLib(raygen.shader_->GetBlob()->GetBufferPointer(), raygen.shader_->GetBlob()->GetBufferSize());
			pLib->SetDXILLibrary(&bcLib);
			// NOTE : エントリ名を指定してコンパイルしているはずだがSetDXILLibraryだけだとファイル内のエクスポートが全部登録されてしまう
			// 毎回明示しないと登録が重複してしまう
			pLib->DefineExport(raygen.shader_->GetEntryName().c_str());
			rayGens_.push_back(raygen);

			if (raygen.localRootSignature_) {
				auto pLocalRootSig = stateObjectDesc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
				pLocalRootSig->SetRootSignature(raygen.localRootSignature_->GetRootSignature().Get());

				auto association = stateObjectDesc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
				association->SetSubobjectToAssociate(*pLocalRootSig);
				association->AddExport(raygen.shader_->GetEntryName().c_str());
			}
		}

		for (auto miss : rtDesc.misses_) {
			auto pLib = stateObjectDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
			CD3DX12_SHADER_BYTECODE bcLib(miss.shader_->GetBlob()->GetBufferPointer(), miss.shader_->GetBlob()->GetBufferSize());
			pLib->SetDXILLibrary(&bcLib);
			pLib->DefineExport(miss.shader_->GetEntryName().c_str());
			misses_.push_back(miss);

			if (miss.localRootSignature_) {
				auto pLocalRootSig = stateObjectDesc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
				pLocalRootSig->SetRootSignature(miss.localRootSignature_->GetRootSignature().Get());

				auto association = stateObjectDesc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
				association->SetSubobjectToAssociate(*pLocalRootSig);
				association->AddExport(miss.shader_->GetEntryName().c_str());
			}
		}

		for (auto hitGroup : rtDesc.hitGroups_) {
			auto pClosestLib = stateObjectDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
			CD3DX12_SHADER_BYTECODE closestBcLib(hitGroup.closestHit_->GetBlob()->GetBufferPointer(), hitGroup.closestHit_->GetBlob()->GetBufferSize());
			pClosestLib->SetDXILLibrary(&closestBcLib);
			pClosestLib->DefineExport(hitGroup.closestHit_->GetEntryName().c_str());
			if (hitGroup.anyHit_) {
				auto pAnyLib = stateObjectDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
				CD3DX12_SHADER_BYTECODE anyBcLib(hitGroup.anyHit_->GetBlob()->GetBufferPointer(), hitGroup.anyHit_->GetBlob()->GetBufferSize());
				pAnyLib->SetDXILLibrary(&anyBcLib);
				pAnyLib->DefineExport(hitGroup.anyHit_->GetEntryName().c_str());
			}
			if (hitGroup.intersection_) {
				auto pItsLib = stateObjectDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
				CD3DX12_SHADER_BYTECODE intersectionBcLib(hitGroup.intersection_->GetBlob()->GetBufferPointer(), hitGroup.intersection_->GetBlob()->GetBufferSize());
				pItsLib->SetDXILLibrary(&intersectionBcLib);
				pItsLib->DefineExport(hitGroup.intersection_->GetEntryName().c_str());
			}

			auto pHitGroup = stateObjectDesc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
			pHitGroup->SetClosestHitShaderImport(hitGroup.closestHit_->GetEntryName().c_str());
			if (hitGroup.anyHit_) {
				pHitGroup->SetAnyHitShaderImport(hitGroup.anyHit_->GetEntryName().c_str());
			}
			if (hitGroup.intersection_) {
				pHitGroup->SetIntersectionShaderImport(hitGroup.intersection_->GetEntryName().c_str());
			}
			pHitGroup->SetHitGroupExport(hitGroup.groupName_.c_str());

			hitGroups_.push_back(hitGroup);

			if (hitGroup.localRootSignature_) {
				auto pLocalRootSig = stateObjectDesc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
				pLocalRootSig->SetRootSignature(hitGroup.localRootSignature_->GetRootSignature().Get());

				auto association = stateObjectDesc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
				association->SetSubobjectToAssociate(*pLocalRootSig);
				association->AddExport(hitGroup.groupName_.c_str());
			}
		}

		auto pShaderConfig = stateObjectDesc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
		// payload size, setting for intersection
		pShaderConfig->Config(rtDesc.rayConfigDesc_.payloadSize_, rtDesc.rayConfigDesc_.attributeSize_);

		auto pPipelineConfig = stateObjectDesc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
		// ray depth
		pPipelineConfig->Config(rtDesc.rayConfigDesc_.rayDepth_);
		HRESULT result = device.GetStableDevice()->CreateStateObject(stateObjectDesc, IID_PPV_ARGS(stateObject_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[StateObject::CreateStateObject] CreateStateObject for raytracing failed\nHRESULT = " + to_string(result));
		}
	}

	void StateObject::CreateStateObject(const Device& device, const StateObjectDesc::WorkGraphDesc& wgDesc)
	{
		CD3DX12_STATE_OBJECT_DESC stateObjectDesc;

		globalRootSig_ = wgDesc.globalRootSig_;
		workGraphProgramName_ = wgDesc.workGraphProgramName_;
		soType_ = StateObjectType::WorkGraph;

		if (wgDesc.graphicsProgramDescs_.size() > 0) {
			hasGraphicsNodes_ = true;
		}
		stateObjectDesc.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_EXECUTABLE);
		auto pSoConfig = stateObjectDesc.CreateSubobject<CD3DX12_STATE_OBJECT_CONFIG_SUBOBJECT>();
		if (hasGraphicsNodes_) {
			// グラフィックスノードがあるときは、グローバルルート引数を Graphics 側から取る
			pSoConfig->SetFlags(D3D12_STATE_OBJECT_FLAG_WORK_GRAPHS_USE_GRAPHICS_STATE_FOR_GLOBAL_ROOT_SIGNATURE);
		}

		if (wgDesc.globalRootSig_) {
			auto pGlobalRootSig = stateObjectDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
			pGlobalRootSig->SetRootSignature(wgDesc.globalRootSig_->GetRootSignature().Get());
		}

		map<wstring, RootSignatureHandle> exportedShaders;
		auto setNodeToStateObject = [&](const ShaderHandle& shader, const RootSignatureHandle& localRootSignature)
			{
				auto it = exportedShaders.find(shader->GetEntryName());
				if (it != exportedShaders.end()) {
					if (it->second != localRootSignature) {
						throw runtime_error("[StateObject::CreateStateObject] a shader shared by multiple nodes must have the same local root signature");
					}
					return;
				}
				exportedShaders.emplace(shader->GetEntryName(), localRootSignature);
				auto pLib = stateObjectDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
				CD3DX12_SHADER_BYTECODE bcLib(shader->GetBlob()->GetBufferPointer(), shader->GetBlob()->GetBufferSize());
				pLib->SetDXILLibrary(&bcLib);
				pLib->DefineExport(shader->GetEntryName().c_str());

				if (localRootSignature) {
					auto pLocalRootSig = stateObjectDesc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
					pLocalRootSig->SetRootSignature(localRootSignature->GetRootSignature().Get());

					auto association = stateObjectDesc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
					association->SetSubobjectToAssociate(*pLocalRootSig);
					association->AddExport(shader->GetEntryName().c_str());
				}
			};

		for (const auto& computeNodeDesc : wgDesc.computeNodeDescs_) {
			computeNodes_.push_back(computeNodeDesc);
			setNodeToStateObject(computeNodeDesc.shader_, computeNodeDesc.localRootSignature_);
		}

		for (const auto& graphicsProgramDescs : wgDesc.graphicsProgramDescs_) {
			graphicsNodes_.push_back(graphicsProgramDescs);
			setNodeToStateObject(graphicsProgramDescs.meshShader_, graphicsProgramDescs.localRootSignature_);
			if (graphicsProgramDescs.pixelShader_ == nullptr) {
				throw runtime_error("[StateObject::CreateStateObject] pixelShader_ of a graphics program must not be nullptr");
			}
			setNodeToStateObject(graphicsProgramDescs.pixelShader_, graphicsProgramDescs.localRootSignature_);
		}

		// 描画の設定はプログラムごとに RenderState からサブオブジェクトを作り、
		// そのプログラムにだけ AddSubobject する（全プログラム共通の既定値は持たない）
		for (const auto& programDesc : wgDesc.graphicsProgramDescs_) {
			auto pGenericProgram = stateObjectDesc.CreateSubobject<CD3DX12_GENERIC_PROGRAM_SUBOBJECT>();
			pGenericProgram->SetProgramName(programDesc.programName_.c_str());
			pGenericProgram->AddExport(programDesc.meshShader_->GetEntryName().c_str());
			pGenericProgram->AddExport(programDesc.pixelShader_->GetEntryName().c_str());

			const auto& renderState = programDesc.renderState_;

			auto pPrimitiveTopology = stateObjectDesc.CreateSubobject<CD3DX12_PRIMITIVE_TOPOLOGY_SUBOBJECT>();
			pPrimitiveTopology->SetPrimitiveTopologyType(renderState.topologyType_);

			auto pRTFormats = stateObjectDesc.CreateSubobject<CD3DX12_RENDER_TARGET_FORMATS_SUBOBJECT>();
			pRTFormats->SetNumRenderTargets(static_cast<UINT>(renderState.rtvFormats_.size()));
			if (renderState.rtvFormats_.size() > D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT) {
				throw runtime_error("[StateObject::CreateStateObject] rtvFormats_ of a program has more than 8 formats");
			}
			for (UINT i = 0; i < renderState.rtvFormats_.size(); i++) {
				pRTFormats->SetRenderTargetFormat(i, renderState.rtvFormats_[i]);
			}

			// 深度バッファの形式。宣言しないと「深度ユニットは DSV を要求しているのに
			// 未宣言」という警告 (STATE_CREATION WARNING #1414) が出る
			auto pDSFormat = stateObjectDesc.CreateSubobject<CD3DX12_DEPTH_STENCIL_FORMAT_SUBOBJECT>();
			pDSFormat->SetDepthStencilFormat(renderState.dsvFormat_);

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
			auto pRasterizer = stateObjectDesc.CreateSubobject<CD3DX12_RASTERIZER_SUBOBJECT>();
			setRasterizer(pRasterizer, renderState.rasterizerDesc_);

			auto pBlend = stateObjectDesc.CreateSubobject<CD3DX12_BLEND_SUBOBJECT>();
			pBlend->SetAlphaToCoverageEnable(renderState.blendState_.AlphaToCoverageEnable);
			pBlend->SetIndependentBlendEnable(renderState.blendState_.IndependentBlendEnable);
			for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++) {
				pBlend->SetRenderTarget(i, renderState.blendState_.RenderTarget[i]);
			}

			auto pDepthStencil = stateObjectDesc.CreateSubobject<CD3DX12_DEPTH_STENCIL_SUBOBJECT>();
			pDepthStencil->SetDepthEnable(renderState.depthStencilDesc_.DepthEnable);
			pDepthStencil->SetDepthWriteMask(renderState.depthStencilDesc_.DepthWriteMask);
			pDepthStencil->SetDepthFunc(renderState.depthStencilDesc_.DepthFunc);
			pDepthStencil->SetStencilEnable(renderState.depthStencilDesc_.StencilEnable);
			pDepthStencil->SetStencilReadMask(renderState.depthStencilDesc_.StencilReadMask);
			pDepthStencil->SetStencilWriteMask(renderState.depthStencilDesc_.StencilWriteMask);
			pDepthStencil->SetFrontFace(renderState.depthStencilDesc_.FrontFace);
			pDepthStencil->SetBackFace(renderState.depthStencilDesc_.BackFace);

			auto pSampleMask = stateObjectDesc.CreateSubobject<CD3DX12_SAMPLE_MASK_SUBOBJECT>();
			pSampleMask->SetSampleMask(renderState.sampleMask_);

			auto pSampleDesc = stateObjectDesc.CreateSubobject<CD3DX12_SAMPLE_DESC_SUBOBJECT>();
			pSampleDesc->SetCount(renderState.sampleDesc_.Count);
			pSampleDesc->SetQuality(renderState.sampleDesc_.Quality);
				
			pGenericProgram->AddSubobject(*pPrimitiveTopology);
			pGenericProgram->AddSubobject(*pRTFormats);
			pGenericProgram->AddSubobject(*pDSFormat);
			pGenericProgram->AddSubobject(*pRasterizer);
			pGenericProgram->AddSubobject(*pBlend);
			pGenericProgram->AddSubobject(*pDepthStencil);
			pGenericProgram->AddSubobject(*pSampleMask);
			pGenericProgram->AddSubobject(*pSampleDesc);
				
		}

		auto pWorkGraph = stateObjectDesc.CreateSubobject<CD3DX12_WORK_GRAPH_SUBOBJECT>();
		pWorkGraph->IncludeAllAvailableNodes();
		pWorkGraph->SetProgramName(wgDesc.workGraphProgramName_.c_str());

		if (hasGraphicsNodes_) {
			for (const auto& computeNodeDesc : wgDesc.computeNodeDescs_) {
				auto pComputeNode = pWorkGraph->CreateShaderNode(computeNodeDesc.shader_->GetEntryName().c_str());
			}
			for (const auto& programDesc : wgDesc.graphicsProgramDescs_) {
				auto pGraphicsNode = pWorkGraph->CreateProgramNode(programDesc.programName_.c_str());
			}
		}

		HRESULT result = device.GetLatestDevice()->CreateStateObject(stateObjectDesc, IID_PPV_ARGS(stateObject_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[StateObject::CreateStateObject] CreateStateObject for work graph failed\nHRESULT = " + to_string(result));
		}
	}

	StateObject::StateObject(const Device& device, wstring name, const StateObjectDesc soDesc)
		: name_(name)
	{
		visit([this, &device](const auto& desc) { CreateStateObject(device, desc); }, soDesc.typeDesc_);
	}

	const ComPtr<ID3D12StateObject>& StateObject::GetStateObject() const
	{
		return stateObject_;
	}

	StateObjectType StateObject::GetStateObjectType() const
	{
		return soType_;
	}

	const wstring& StateObject::GetWorkGraphProgramName() const
	{
		if (!workGraphProgramName_.empty()) {
			// **値で受けてはいけない。** ローカルのメンバへの参照を返すことになり、
			// 関数を抜けた時点でダングリングする(warning C4172)
			return workGraphProgramName_;
		}
		else {
			throw runtime_error("[StateObject::GetWorkGraphProgramName] a raytracing state object has no program name");
		}
	}

	const vector<StateObjectDesc::ShaderExportDesc>& StateObject::GetRayGens() const
	{
		return rayGens_;
	}

	const vector<StateObjectDesc::ShaderExportDesc>& StateObject::GetMisses() const
	{
		return misses_;
	}

	const vector<StateObjectDesc::HitGroupExportDesc>& StateObject::GetHitGroups() const
	{
		return hitGroups_;
	}

	const std::vector<StateObjectDesc::ShaderExportDesc>& StateObject::GetComputeNodes() const
	{
		return computeNodes_;
	}

	const std::vector<StateObjectDesc::ProgramDesc>& StateObject::GetGraphicsNodes() const
	{
		return graphicsNodes_;
	}

	bool StateObject::HasGraphicsNodes() const
	{
		return hasGraphicsNodes_;
	}
}
