#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

Indirect::Indirect()
{
	
}

bool Indirect::Init(Device* pDevice)
{
	pDevice_ = pDevice;
	if (pDevice_ == nullptr) {
		cerr << "Indirect class doesn't have any pointer" << endl;
		return false;
	}

	return true;
}

bool Indirect::InitializeCommandSignature(const RootSignature& rootSignature, UINT byteStride)
{
	cmdSigDesc_.pArgumentDescs = indirectArgDesc_.data();
	cmdSigDesc_.NumArgumentDescs = indirectArgDesc_.size();
	cmdSigDesc_.ByteStride = byteStride;
	cmdSigDesc_.NodeMask = 0;

	auto result = pDevice_->GetDevice()->CreateCommandSignature(&cmdSigDesc_, rootSignature.GetRootSignature().Get(), IID_PPV_ARGS(cmdSig_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		cerr << "Failed to CreateCommandSignaure" << endl;
		return false;
	}

	return true;
}

bool Indirect::InitializeCommandSignature(UINT byteStride)
{
	cmdSigDesc_.pArgumentDescs = indirectArgDesc_.data();
	cmdSigDesc_.NumArgumentDescs = indirectArgDesc_.size();
	cmdSigDesc_.ByteStride = byteStride;
	cmdSigDesc_.NodeMask = 0;

	auto result = pDevice_->GetDevice()->CreateCommandSignature(&cmdSigDesc_, nullptr, IID_PPV_ARGS(cmdSig_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		cerr << "Failed to CreateCommandSignaure" << endl;
		return false;
	}

	return true;
}

// not numReg but RootParameterIndex
void Indirect::AddCBV(UINT rootParameterIndex)
{
	D3D12_INDIRECT_ARGUMENT_DESC desc;
	desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
	desc.ConstantBufferView.RootParameterIndex = rootParameterIndex;
	indirectArgDesc_.push_back(desc);
}

void Indirect::AddSRV(UINT rootParameterIndex)
{
	D3D12_INDIRECT_ARGUMENT_DESC desc;
	desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
	desc.ShaderResourceView.RootParameterIndex = rootParameterIndex;
	indirectArgDesc_.push_back(desc);
}

void Indirect::AddUAV(UINT rootParameterIndex)
{
	D3D12_INDIRECT_ARGUMENT_DESC desc;
	desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
	desc.UnorderedAccessView.RootParameterIndex = rootParameterIndex;
	indirectArgDesc_.push_back(desc);
}

void Indirect::AddVertexBufferView(UINT slot)
{
	D3D12_INDIRECT_ARGUMENT_DESC desc;
	desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
	desc.VertexBuffer.Slot = slot;
	indirectArgDesc_.push_back(desc);
}

void Indirect::AddIndexBufferView()
{
	D3D12_INDIRECT_ARGUMENT_DESC desc;
	desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
	indirectArgDesc_.push_back(desc);
}

void Indirect::AddDrawIndexed()
{
	D3D12_INDIRECT_ARGUMENT_DESC desc;
	desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
	indirectArgDesc_.push_back(desc);
}

void Indirect::AddDraw()
{
	D3D12_INDIRECT_ARGUMENT_DESC desc;
	desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
	indirectArgDesc_.push_back(desc);
}

void Indirect::AddDispatch()
{
	D3D12_INDIRECT_ARGUMENT_DESC desc;
	desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
	indirectArgDesc_.push_back(desc);
}

ComPtr<ID3D12CommandSignature> Indirect::GetCommandSignature() const
{
	return cmdSig_;
}