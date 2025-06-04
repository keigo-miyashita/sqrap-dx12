#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

RootSignature::RootSignature(const Device& device, D3D12_ROOT_SIGNATURE_FLAGS flag, std::initializer_list<RootParameter> rootParams, std::wstring name)
	: pDevice_(&device), flag_(flag), name_(name)
{
	for (auto rootParam_ : rootParams) {
		CD3DX12_ROOT_PARAMETER rp;
		if (rootParam_.rootParamType_ == RootParamType::DescTable) {
			DescTableRootParamDesc descManagerRef = std::get<DescTableRootParamDesc>(rootParam_.rootParamDesc_);
			rp.InitAsDescriptorTable(descManagerRef.descManager.GetNumDescRanges(), descManagerRef.descManager.GetPDescRanges(), rootParam_.shaderVisibility_);
			rps_.push_back(rp);
			size_ += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
		}
		else if (rootParam_.rootParamType_ == RootParamType::CBV) {
			DirectRootParamDesc directDesc = std::get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
			rp.InitAsConstantBufferView(directDesc.numReg, rootParam_.shaderVisibility_);
			rps_.push_back(rp);
			size_ += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
		}
		else if (rootParam_.rootParamType_ == RootParamType::SRV) {
			DirectRootParamDesc directDesc = std::get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
			rp.InitAsShaderResourceView(directDesc.numReg, rootParam_.shaderVisibility_);
			rps_.push_back(rp);
			size_ += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
		}
		else if (rootParam_.rootParamType_ == RootParamType::UAV) {
			DirectRootParamDesc directDesc = std::get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
			rp.InitAsUnorderedAccessView(directDesc.numReg, rootParam_.shaderVisibility_);
			rps_.push_back(rp);
			size_ += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
		}
		else if (rootParam_.rootParamType_ == RootParamType::Constant) {
			DirectRootParamDesc directDesc = std::get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
			rp.InitAsConstants(directDesc.numConstant, directDesc.numReg, rootParam_.shaderVisibility_);
			rps_.push_back(rp);
			// For padding
			// SBT needs 8 byte alignment for each parameter
			if (size_ % 8 != 0) {
				size_ += 4;
			}
			size_ += 4;
		}
	}

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	// NOTE : 
	// 第3引数はSampler関係 Samplerリソースは別のメソッド定義が必要？
	rootSignatureDesc.Init(rps_.size(), rps_.data(), 0, nullptr, flag_);

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

UINT RootSignature::GetSize() const
{
	return size_;
}

const std::vector<CD3DX12_ROOT_PARAMETER>& RootSignature::GetRootParameters() const
{
	return rps_;
}

ResourceSet::ResourceSet(std::shared_ptr<RootSignature> pRootSignature, std::initializer_list<std::variant<DescriptorManager, std::shared_ptr<Buffer>, Constants>> bindedResources)
	: pRootSignature_(pRootSignature)
{
	cout << "Make ResourceSet" << endl;
	for (auto bindedResource : bindedResources) {
		if (std::holds_alternative<DescriptorManager>(bindedResource)) {
			DescriptorManager dm = std::get<DescriptorManager>(bindedResource);
			bindedResources_.push_back(dm.GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
			descriptorManagers_.push_back(dm);
		}
		else if (std::holds_alternative<std::shared_ptr<Buffer>>(bindedResource)) {
			std::shared_ptr<Buffer> b = std::get<std::shared_ptr<Buffer>>(bindedResource);
			bindedResources_.push_back(b->GetGPUAddress());
		}
		else if (std::holds_alternative<Constants>(bindedResource)) {
			Constants c = std::get<Constants>(bindedResource);
			bindedResources_.push_back(c);
		}
	}
}

std::shared_ptr<RootSignature> ResourceSet::GetRootSignature() const
{
	return pRootSignature_;
}

const std::vector<std::variant<D3D12_GPU_DESCRIPTOR_HANDLE, D3D12_GPU_VIRTUAL_ADDRESS, Constants>> ResourceSet::GetBindedResources() const
{
	return bindedResources_;
}

const std::vector<DescriptorManager> ResourceSet::GetDescManagers() const
{
	return descriptorManagers_;
}