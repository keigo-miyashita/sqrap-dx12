#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

RootParameter::RootParameter()
{

}

void RootParameter::InitAsDescriptorTable(UINT descTableSize, const CD3DX12_DESCRIPTOR_RANGE* pDescTable, D3D12_SHADER_VISIBILITY shaderVisibility)
{
	rootParamType_ = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	descTableSize_ = descTableSize;
	pDescTable_ = pDescTable;
	shaderVisibility_ = shaderVisibility;
}

void RootParameter::InitAsConstantBufferView(UINT numReg, D3D12_SHADER_VISIBILITY shaderVisibility)
{
	rootParamType_ = D3D12_ROOT_PARAMETER_TYPE_CBV;
	numReg_ = numReg;
	shaderVisibility_ = shaderVisibility;
}

void RootParameter::InitAsShaderResourceView(UINT numReg, D3D12_SHADER_VISIBILITY shaderVisibility)
{
	rootParamType_ = D3D12_ROOT_PARAMETER_TYPE_SRV;
	numReg_ = numReg;
	shaderVisibility_ = shaderVisibility;
}

void RootParameter::InitAsUnorderedAccessView(UINT numReg, D3D12_SHADER_VISIBILITY shaderVisibility)
{
	rootParamType_ = D3D12_ROOT_PARAMETER_TYPE_UAV;
	numReg_ = numReg;
	shaderVisibility_ = shaderVisibility;
}

void RootParameter::InitAs32BitConstant(UINT numReg, UINT numConstant)
{
	rootParamType_ = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	numReg_ = numReg;
	numConstant_ = numConstant;
}

D3D12_ROOT_PARAMETER_TYPE RootParameter::GetRootParamType()
{
	return rootParamType_;
}

UINT RootParameter::GetDescTableSize()
{
	return descTableSize_;
}

const CD3DX12_DESCRIPTOR_RANGE* RootParameter::GetPDescTable() const
{
	return pDescTable_;
}

UINT RootParameter::GetNumReg()
{
	return numReg_;
}

D3D12_SHADER_VISIBILITY RootParameter::GetShaderVisibility()
{
	return shaderVisibility_;
}

UINT RootParameter::GetNumConstant()
{
	return numConstant_;
}

bool RootSignature::InitializeRootSignature(D3D12_ROOT_SIGNATURE_FLAGS flags, wstring name)
{
	std::vector<CD3DX12_ROOT_PARAMETER> rps;
	for (auto rootParam_ : rootParams_) {
		CD3DX12_ROOT_PARAMETER rp;
		if (rootParam_.GetRootParamType() == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
			rp.InitAsDescriptorTable(rootParam_.GetDescTableSize(), rootParam_.GetPDescTable(), rootParam_.GetShaderVisibility());
			rps.push_back(rp);
		}
		else if (rootParam_.GetRootParamType() == D3D12_ROOT_PARAMETER_TYPE_CBV) {
			rp.InitAsConstantBufferView(rootParam_.GetNumReg(), rootParam_.GetShaderVisibility());
			rps.push_back(rp);
		}
		else if (rootParam_.GetRootParamType() == D3D12_ROOT_PARAMETER_TYPE_SRV) {
			rp.InitAsShaderResourceView(rootParam_.GetNumReg(), rootParam_.GetShaderVisibility());
			rps.push_back(rp);
		}
		else if (rootParam_.GetRootParamType() == D3D12_ROOT_PARAMETER_TYPE_UAV) {
			rp.InitAsUnorderedAccessView(rootParam_.GetNumReg(), rootParam_.GetShaderVisibility());
			rps.push_back(rp);
		}
		else if (rootParam_.GetRootParamType() == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS) {
			rp.InitAsConstants(rootParam_.GetNumConstant(), rootParam_.GetNumReg(), rootParam_.GetShaderVisibility());
			rps.push_back(rp);
		}
	}

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	// NOTE : 
	// 第3引数はSampler関係 Samplerリソースは別のメソッド定義が必要？
	rootSignatureDesc.Init(rps.size(), rps.data(), 0, nullptr, flags);

	ComPtr<ID3DBlob> rootSigBlob = nullptr;
	ComPtr<ID3DBlob> errBlob = nullptr;
	auto result = D3D12SerializeRootSignature(&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		rootSigBlob.ReleaseAndGetAddressOf(),
		errBlob.ReleaseAndGetAddressOf());
	if (FAILED(result)) {
		cerr << "Failed to serialize root signature" << endl;
		if (errBlob) {
			OutputDebugStringA((char*)errBlob->GetBufferPointer());
		}
		return false;
	}
	
	if (FAILED(pDevice_->GetDevice()->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(rootSig_.ReleaseAndGetAddressOf())))) {
		cerr << "Failed to create root signature" << endl;
		return false;
	}

	rootSig_->SetName(name.c_str());

	return true;
}

RootSignature::RootSignature()
{

}

bool RootSignature::Init(Device* pDevice)
{
	pDevice_ = pDevice;
	return true;
}

void RootSignature::AddDescriptorTable(const DescriptorTable& descriptorTable, D3D12_SHADER_VISIBILITY shaderVisibility)
{
	RootParameter rootParam;
	rootParam.InitAsDescriptorTable(descriptorTable.GetNumDescRanges(), descriptorTable.GetPDescRanges(), shaderVisibility);
	rootParams_.push_back(rootParam);
}

void RootSignature::AddDescriptor(D3D12_ROOT_PARAMETER_TYPE rootParamType, UINT numReg, D3D12_SHADER_VISIBILITY shaderVisibility)
{
	RootParameter rootParam;
	if (rootParamType == D3D12_ROOT_PARAMETER_TYPE_CBV) {
		rootParam.InitAsConstantBufferView(numReg, shaderVisibility);
	}
	else if (rootParamType == D3D12_ROOT_PARAMETER_TYPE_SRV) {
		rootParam.InitAsShaderResourceView(numReg, shaderVisibility);
	}
	else if (rootParamType == D3D12_ROOT_PARAMETER_TYPE_UAV) {
		rootParam.InitAsUnorderedAccessView(numReg, shaderVisibility);
	}
	rootParams_.push_back(rootParam);
}

void RootSignature::AddConstant(UINT numReg, UINT numConstant)
{
	RootParameter rootParam;
	rootParam.InitAs32BitConstant(numReg, numConstant);
	rootParams_.push_back(rootParam);
}

ComPtr<ID3D12RootSignature> RootSignature::GetRootSignature() const
{
	return rootSig_;
}
