#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

RootSignature::RootSignature(const Device& device, D3D12_ROOT_SIGNATURE_FLAGS flag, std::initializer_list<RootParameter> rootParams, std::wstring name)
	: pDevice_(&device), flag_(flag), name_(name)
{
	std::vector<CD3DX12_ROOT_PARAMETER> rps;
	for (auto rootParam_ : rootParams) {
		CD3DX12_ROOT_PARAMETER rp;
		if (rootParam_.rootParamType_ == RootParamType::DescTable) {
			DescTableRootParamDesc descManagerRef = std::get<DescTableRootParamDesc>(rootParam_.rootParamDesc_);
			rp.InitAsDescriptorTable(descManagerRef.descManager.GetNumDescRanges(), descManagerRef.descManager.GetPDescRanges(), rootParam_.shaderVisibility_);
			rps.push_back(rp);
		}
		else if (rootParam_.rootParamType_ == RootParamType::CBV) {
			DirectRootParamDesc directDesc = std::get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
			rp.InitAsConstantBufferView(directDesc.numReg, rootParam_.shaderVisibility_);
			rps.push_back(rp);
		}
		else if (rootParam_.rootParamType_ == RootParamType::SRV) {
			DirectRootParamDesc directDesc = std::get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
			rp.InitAsShaderResourceView(directDesc.numReg, rootParam_.shaderVisibility_);
			rps.push_back(rp);
		}
		else if (rootParam_.rootParamType_ == RootParamType::UAV) {
			DirectRootParamDesc directDesc = std::get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
			rp.InitAsUnorderedAccessView(directDesc.numReg, rootParam_.shaderVisibility_);
			rps.push_back(rp);
		}
		else if (rootParam_.rootParamType_ == RootParamType::Constant) {
			DirectRootParamDesc directDesc = std::get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
			rp.InitAsConstants(directDesc.numConstant, directDesc.numReg, rootParam_.shaderVisibility_);
			rps.push_back(rp);
		}
	}

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	// NOTE : 
	// 第3引数はSampler関係 Samplerリソースは別のメソッド定義が必要？
	rootSignatureDesc.Init(rps.size(), rps.data(), 0, nullptr, flag_);

	ComPtr<ID3DBlob> rootSigBlob = nullptr;
	ComPtr<ID3DBlob> errBlob = nullptr;
	HRESULT result = D3D12SerializeRootSignature(&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		rootSigBlob.ReleaseAndGetAddressOf(),
		errBlob.ReleaseAndGetAddressOf());
	if (FAILED(result)) {
		throw std::runtime_error("Failed to D3D12SerializeRootSignature : " + to_string(result));
		if (errBlob) {
			OutputDebugStringA((char*)errBlob->GetBufferPointer());
		}
	}

	result = pDevice_->GetDevice()->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(rootSig_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		throw std::runtime_error("Failed to CreateRootSignature : " + to_string(result));
	}

	rootSig_->SetName(name.c_str());
}

ComPtr<ID3D12RootSignature> RootSignature::GetRootSignature() const
{
	return rootSig_;
}
