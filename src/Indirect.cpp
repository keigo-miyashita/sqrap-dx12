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
		const RootSignatureHandle& rootSignature,
		UINT byteStride,
		UINT maxCommandCount,
		const BufferHandle& argumentBuffer,
		const BufferHandle& counterBuffer
	) : pDevice_(&device), name_(name), maxCommandCount_(maxCommandCount), argumentBuffer_(argumentBuffer), counterBuffer_(counterBuffer)
	{
		for (auto indirectDesc : indirectDescs) {
			D3D12_INDIRECT_ARGUMENT_DESC desc = {};
			if (indirectDesc.type_ == IndirectType::Draw) {
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
				indirectArgDesc_.push_back(desc);
			}
			else if (indirectDesc.type_ == IndirectType::DrawIndexed) {
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
				indirectArgDesc_.push_back(desc);
			}
			else if (indirectDesc.type_ == IndirectType::Dispatch) {
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
				indirectArgDesc_.push_back(desc);
			}
			else if (indirectDesc.type_ == IndirectType::VB) {
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
				desc.VertexBuffer.Slot = 0;
				indirectArgDesc_.push_back(desc);
			}
			else if (indirectDesc.type_ == IndirectType::IB) {
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
				indirectArgDesc_.push_back(desc);
			}
			else if (indirectDesc.type_ == IndirectType::Constant) {
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
				desc.Constant.RootParameterIndex = indirectDesc.rootParameterIndex_;
				desc.Constant.DestOffsetIn32BitValues = 0;
				desc.Constant.Num32BitValuesToSet = indirectDesc.numConstant_;
				indirectArgDesc_.push_back(desc);
			}
			else if (indirectDesc.type_ == IndirectType::CBV) {
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
				desc.ConstantBufferView.RootParameterIndex = indirectDesc.rootParameterIndex_;
				indirectArgDesc_.push_back(desc);
			}
			else if (indirectDesc.type_ == IndirectType::SRV) {
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
				desc.ShaderResourceView.RootParameterIndex = indirectDesc.rootParameterIndex_;
				indirectArgDesc_.push_back(desc);
			}
			else if (indirectDesc.type_ == IndirectType::UAV) {
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
				desc.UnorderedAccessView.RootParameterIndex = indirectDesc.rootParameterIndex_;
				indirectArgDesc_.push_back(desc);
			}
			else if (indirectDesc.type_ == IndirectType::Ray) {
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS;
				indirectArgDesc_.push_back(desc);
			}
			else if (indirectDesc.type_ == IndirectType::Mesh) {
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;
				indirectArgDesc_.push_back(desc);
			}
		}

		cmdSigDesc_.pArgumentDescs = indirectArgDesc_.data();
		cmdSigDesc_.NumArgumentDescs = indirectArgDesc_.size();
		cmdSigDesc_.ByteStride = byteStride;
		cmdSigDesc_.NodeMask = 0;

		HRESULT result;
		if (rootSignature) {
			result = pDevice_->GetDevice()->CreateCommandSignature(&cmdSigDesc_, rootSignature->GetRootSignature().Get(), IID_PPV_ARGS(cmdSig_.ReleaseAndGetAddressOf()));
		}
		else {
			result = pDevice_->GetDevice()->CreateCommandSignature(&cmdSigDesc_, nullptr, IID_PPV_ARGS(cmdSig_.ReleaseAndGetAddressOf()));
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

	UINT Indirect::GetMaxCommandCount() const
	{
		return maxCommandCount_;
	}

	BufferHandle Indirect::GetArgumentBuffer() const
	{
		return argumentBuffer_;
	}
	BufferHandle Indirect::GetCounterBuffer() const
	{
		return counterBuffer_;
	}

	UINT64 Indirect::GetCounterBufferOffset() const
	{
		return counterBuffer_->GetOffsetCounter();
	}
}