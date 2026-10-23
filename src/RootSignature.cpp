#include "RootSignature.hpp"

#include "Device.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	RootSignature::RootSignature(const Device& device, wstring name, D3D12_ROOT_SIGNATURE_FLAGS flag, initializer_list<RootParameter> rootParams)
		: pDevice_(&device), flag_(flag), rootParams_(rootParams), name_(name)
	{
		for (const auto& rootParam_ : rootParams_) {
			CD3DX12_ROOT_PARAMETER1 rp;
			if (rootParam_.rootParamType_ == RootParamType::DescTable) {
				const DescriptorLayout& descLayout = get<DescriptorLayout>(rootParam_.rootParamDesc_);
				for (const auto& reg : descLayout.GetUsedRegs()) {
					if (!usedRegs_.insert(reg).second) {
						throw runtime_error("[RootSignature::RootSignature] duplicate register index in DescriptorLayout");
					}
				}
				if (descLayout.GetResourceDescNum() != 0) {
					rp.InitAsDescriptorTable(descLayout.GetResourceDescNum(), descLayout.GetPResourceDescRanges(), rootParam_.shaderVisibility_);
					rps_.push_back(rp);
					size_ += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
				}
				if (descLayout.GetSamplerDescNum() != 0) {
					rp.InitAsDescriptorTable(descLayout.GetSamplerDescNum(), descLayout.GetPSamplerDescRanges(), rootParam_.shaderVisibility_);
					rps_.push_back(rp);
					size_ += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
				}
			}
			else if (rootParam_.rootParamType_ == RootParamType::CBV) {
				DirectRootParamDesc directDesc = get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
				if (!usedRegs_.insert({ ViewType::CBV, directDesc.numReg_ }).second) {
					throw runtime_error("[RootSignature::RootSignature] duplicate register index in CBV");
				}
				rp.InitAsConstantBufferView(directDesc.numReg_, rootParam_.shaderVisibility_);
				rps_.push_back(rp);
				size_ += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
			}
			else if (rootParam_.rootParamType_ == RootParamType::SRV) {
				DirectRootParamDesc directDesc = get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
				if (!usedRegs_.insert({ ViewType::SRV, directDesc.numReg_ }).second) {
					throw runtime_error("[RootSignature::RootSignature] duplicate register index in SRV");
				}
				rp.InitAsShaderResourceView(directDesc.numReg_, rootParam_.shaderVisibility_);
				rps_.push_back(rp);
				size_ += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
			}
			else if (rootParam_.rootParamType_ == RootParamType::UAV) {
				DirectRootParamDesc directDesc = get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
				if (!usedRegs_.insert({ ViewType::UAV, directDesc.numReg_ }).second) {
					throw runtime_error("[RootSignature::RootSignature] duplicate register index in UAV");
				}
				rp.InitAsUnorderedAccessView(directDesc.numReg_, rootParam_.shaderVisibility_);
				rps_.push_back(rp);
				size_ += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
			}
			else if (rootParam_.rootParamType_ == RootParamType::Constant) {
				DirectRootParamDesc directDesc = get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
				if (!usedRegs_.insert({ ViewType::CBV, directDesc.numReg_ }).second) {
					throw runtime_error("[RootSignature::RootSignature] duplicate register index in root constants");
				}
				rp.InitAsConstants(directDesc.numConstant_, directDesc.numReg_, rootParam_.shaderVisibility_);
				rps_.push_back(rp);
				// For padding
				// SBT needs 8 byte alignment for each parameter
				if (size_ % 8 != 0) {
					size_ += 4;
				}
				size_ += 4;
			}
		}

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		// TODO : 
		// Need another method for support sampler
		// Third argument is for sampler
		rootSignatureDesc.Init_1_1(rps_.size(), rps_.data(), 0, nullptr, flag_);

		ComPtr<ID3DBlob> rootSigBlob = nullptr;
		ComPtr<ID3DBlob> errBlob = nullptr;
		HRESULT result = D3D12SerializeVersionedRootSignature(&rootSignatureDesc,
			rootSigBlob.ReleaseAndGetAddressOf(),
			errBlob.ReleaseAndGetAddressOf());
		if (FAILED(result)) {
			if (errBlob) {
				OutputDebugStringA((char*)errBlob->GetBufferPointer());
			}
			throw runtime_error("[RootSignature::RootSignature] D3D12SerializeRootSignature failed\nHRESULT = " + to_string(result));
		}

		result = pDevice_->GetDevice()->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(rootSig_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[RootSignature::RootSignature] CreateRootSignature failed\nHRESULT = " + to_string(result));
		}

		rootSig_->SetName(name.c_str());
	}

	const ComPtr<ID3D12RootSignature>& RootSignature::GetRootSignature() const
	{
		return rootSig_;
	}

	UINT RootSignature::GetSize() const
	{
		return size_;
	}

	const vector<CD3DX12_ROOT_PARAMETER1>& RootSignature::GetRootParameters() const
	{
		return rps_;
	}

	const vector<RootParameter>& RootSignature::GetRootParametersVec() const
	{
		return rootParams_;
	}

	ResourceSet::ResourceSet(const RootSignatureHandle& rootSignature, const vector<ResourceSetDesc>& resourceSetDescs)
		: rootSignature_(rootSignature), resourceSetDescs_(resourceSetDescs)
	{
	}

	RootSignatureHandle ResourceSet::GetRootSignature() const
	{
		return rootSignature_;
	}

	const vector<ResourceSetDesc>& ResourceSet::GetResourceSetDescs() const
	{
		return resourceSetDescs_;
	}
}