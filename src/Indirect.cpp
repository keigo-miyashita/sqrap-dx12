#include "Indirect.hpp"

#include "Device.hpp"
#include "RootSignature.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	Indirect::Indirect(const Device& device,
		wstring name,
		initializer_list<IndirectDesc> indirectDescs,
		UINT byteStride,
		const RootSignatureHandle& rootSignature
	) : name_(name)
	{
		bool needRootSignature = false;
		UINT minByteStride = 0;
		vector<D3D12_INDIRECT_ARGUMENT_DESC> indirectArgDescs;
		D3D12_COMMAND_SIGNATURE_DESC cmdSigDesc = {};
		const vector<RootParameter>* rootParams = rootSignature ? &rootSignature->GetRootParametersVec() : nullptr;
		const auto& indirectDescsItr = indirectDescs.begin();

		auto checkRootParameterIndex =
			[&](UINT rootParamIndex) {
			if (!rootSignature) {
				throw runtime_error("[Indirect::Indirect] rootSignature is required because an argument changes root arguments (Constant / IncConstant / CBV / SRV / UAV)");
			}

			if (rootParamIndex >= rootParams->size()) {
				throw runtime_error("[Indirect::Indirect] rootParameterIndex_ (" + to_string(rootParamIndex) + ") is out of range of rootSignature (" + to_string(rootParams->size()) + " parameters)");
			}
			};

		for (int i = 0; i < indirectDescs.size(); i++) {
			D3D12_INDIRECT_ARGUMENT_DESC desc = {};
			if (indirectDescsItr[i].type_ == IndirectType::Draw) {
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
				indirectArgDescs.push_back(desc);
				minByteStride += sizeof(D3D12_DRAW_ARGUMENTS);
			}
			else if (indirectDescsItr[i].type_ == IndirectType::DrawIndexed) {
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
				indirectArgDescs.push_back(desc);
				minByteStride += sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
			}
			else if (indirectDescsItr[i].type_ == IndirectType::Dispatch) {
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
				indirectArgDescs.push_back(desc);
				minByteStride += sizeof(D3D12_DISPATCH_ARGUMENTS);
			}
			else if (indirectDescsItr[i].type_ == IndirectType::VB) {
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
				desc.VertexBuffer.Slot = 0;
				indirectArgDescs.push_back(desc);
				minByteStride += sizeof(D3D12_VERTEX_BUFFER_VIEW);
			}
			else if (indirectDescsItr[i].type_ == IndirectType::IB) {
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
				indirectArgDescs.push_back(desc);
				minByteStride += sizeof(D3D12_INDEX_BUFFER_VIEW);
			}
			else if (indirectDescsItr[i].type_ == IndirectType::Constant) {
				const UINT rootParamIndex = indirectDescsItr[i].rootParameterIndex_;
				checkRootParameterIndex(rootParamIndex);
				if ((*rootParams)[rootParamIndex].rootParamType_ != RootParamType::Constant) {
					throw runtime_error("[Indirect::Indirect] indirect desc " + to_string(i) + ": root parameter " + to_string(rootParamIndex) + " is not a root constant");
				}
				auto& directRootParamDesc = std::get<DirectRootParamDesc>((*rootParams)[rootParamIndex].rootParamDesc_);
				if (indirectDescsItr[i].numConstant_ != directRootParamDesc.numConstant_) {
					throw runtime_error("[Indirect::Indirect] indirect desc " + to_string(i) + ": numConstant_ (" + to_string(indirectDescsItr[i].numConstant_) + ") does not match root parameter " + to_string(rootParamIndex) + " (" + to_string(directRootParamDesc.numConstant_) + ")");
				}
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
				desc.Constant.RootParameterIndex = rootParamIndex;
				desc.Constant.DestOffsetIn32BitValues = 0;
				desc.Constant.Num32BitValuesToSet = indirectDescsItr[i].numConstant_;
				minByteStride += sizeof(UINT) * indirectDescsItr[i].numConstant_;
				indirectArgDescs.push_back(desc);
				needRootSignature = true;
			}
			else if (indirectDescsItr[i].type_ == IndirectType::CBV) {
				const UINT rootParamIndex = indirectDescsItr[i].rootParameterIndex_;
				checkRootParameterIndex(rootParamIndex);
				if ((*rootParams)[rootParamIndex].rootParamType_ != RootParamType::CBV) {
					throw runtime_error("[Indirect::Indirect] indirect desc " + to_string(i) + ": root parameter " + to_string(rootParamIndex) + " is not a root CBV");
				}
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
				desc.ConstantBufferView.RootParameterIndex = rootParamIndex;
				indirectArgDescs.push_back(desc);
				minByteStride += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
				needRootSignature = true;
			}
			else if (indirectDescsItr[i].type_ == IndirectType::SRV) {
				const UINT rootParamIndex = indirectDescsItr[i].rootParameterIndex_;
				checkRootParameterIndex(rootParamIndex);
				if ((*rootParams)[rootParamIndex].rootParamType_ != RootParamType::SRV) {
					throw runtime_error("[Indirect::Indirect] indirect desc " + to_string(i) + ": root parameter " + to_string(rootParamIndex) + " is not a root SRV");
				}
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
				desc.ShaderResourceView.RootParameterIndex = rootParamIndex;
				indirectArgDescs.push_back(desc);
				minByteStride += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
				needRootSignature = true;
			}
			else if (indirectDescsItr[i].type_ == IndirectType::UAV) {
				const UINT rootParamIndex = indirectDescsItr[i].rootParameterIndex_;
				checkRootParameterIndex(rootParamIndex);
				if ((*rootParams)[rootParamIndex].rootParamType_ != RootParamType::UAV) {
					throw runtime_error("[Indirect::Indirect] indirect desc " + to_string(i) + ": root parameter " + to_string(rootParamIndex) + " is not a root UAV");
				}
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
				desc.UnorderedAccessView.RootParameterIndex = rootParamIndex;
				indirectArgDescs.push_back(desc);
				minByteStride += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
				needRootSignature = true;
			}
			else if (indirectDescsItr[i].type_ == IndirectType::Ray) {
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS;
				indirectArgDescs.push_back(desc);
				minByteStride += sizeof(D3D12_DISPATCH_RAYS_DESC);
			}
			else if (indirectDescsItr[i].type_ == IndirectType::Mesh) {
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;
				indirectArgDescs.push_back(desc);
				minByteStride += sizeof(D3D12_DISPATCH_MESH_ARGUMENTS);
			}
			else if (indirectDescsItr[i].type_ == IndirectType::IncConstant) {
				const UINT rootParamIndex = indirectDescsItr[i].rootParameterIndex_;
				checkRootParameterIndex(rootParamIndex);
				if ((*rootParams)[rootParamIndex].rootParamType_ != RootParamType::Constant) {
					throw runtime_error("[Indirect::Indirect] indirect desc " + to_string(i) + ": root parameter " + to_string(rootParamIndex) + " is not a root constant");
				}
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_INCREMENTING_CONSTANT;
				desc.IncrementingConstant.RootParameterIndex = indirectDescsItr[i].rootParameterIndex_;
				desc.IncrementingConstant.DestOffsetIn32BitValues = 0;
				indirectArgDescs.push_back(desc);
				minByteStride += 0;
				needRootSignature = true;
			}
			else {
				throw runtime_error("[Indirect::Indirect] invalid IndirectType");
			}
		}

		if (minByteStride > byteStride) {
			throw runtime_error("[Indirect::Indirect] byteStride (" + to_string(byteStride) + ") is smaller than the total size of the arguments (" + to_string(minByteStride) + ")");
		}

		if (rootSignature && !needRootSignature) {
			throw runtime_error("[Indirect::Indirect] rootSignature must be nullptr when no argument changes root arguments (Constant / IncConstant / CBV / SRV / UAV)");
		}

		cmdSigDesc.pArgumentDescs = indirectArgDescs.data();
		cmdSigDesc.NumArgumentDescs = static_cast<UINT>(indirectArgDescs.size());
		cmdSigDesc.ByteStride = byteStride;
		cmdSigDesc.NodeMask = 0;

		HRESULT result;
		if (rootSignature) {
			result = device.GetDevice()->CreateCommandSignature(&cmdSigDesc, rootSignature->GetRootSignature().Get(), IID_PPV_ARGS(cmdSig_.ReleaseAndGetAddressOf()));
		}
		else {
			result = device.GetDevice()->CreateCommandSignature(&cmdSigDesc, nullptr, IID_PPV_ARGS(cmdSig_.ReleaseAndGetAddressOf()));
		}
		if (FAILED(result)) {
			throw runtime_error("[Indirect::Indirect] CreateCommandSignature failed\nHRESULT = " + to_string(result));
		}
		else {
			cmdSig_->SetName((name_ + L"_CommandSignature").c_str());
		}
	}

	const ComPtr<ID3D12CommandSignature>& Indirect::GetCommandSignature() const
	{
		return cmdSig_;
	}
}