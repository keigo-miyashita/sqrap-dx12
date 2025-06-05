#pragma once

#include <common.hpp>

class DescriptorManager;
class Device;

enum class RootParamType
{
	DescTable, CBV, SRV, UAV, Constant,
};

struct DescTableRootParamDesc
{
	const DescriptorManager& descManager;
};

struct DirectRootParamDesc
{
	// Constant‚âDescriptor‚ð’¼Ú“o˜^‚·‚é‚Æ‚«‰º‚Ì1‚Â
	UINT numReg = 0;
	// 32BitsConstant—p
	// 1‚Â32Bits = 4Bytes
	// —ájfloat4 -> 16 Bytes -> numConstant_ = 4
	UINT numConstant = 0;
};

struct RootParameter
{
	RootParamType rootParamType_;
	// Descriptor table‚ð“o˜^‚·‚é‚Æ‚«‰º‚Ì1‚Â
	std::variant<DescTableRootParamDesc, DirectRootParamDesc> rootParamDesc_;
	D3D12_SHADER_VISIBILITY shaderVisibility_ = D3D12_SHADER_VISIBILITY_ALL;
};

class RootSignature
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const Device* pDevice_ = nullptr;
	ComPtr<ID3D12RootSignature> rootSig_ = nullptr;
	D3D12_ROOT_SIGNATURE_FLAGS flag_ = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	std::vector<CD3DX12_ROOT_PARAMETER> rps_;
	std::wstring name_;
	UINT size_ = 0;

public:
	RootSignature(const Device& device, D3D12_ROOT_SIGNATURE_FLAGS flag, std::initializer_list<RootParameter> rootParams, std::wstring name = L"");
	~RootSignature() = default;
	ComPtr<ID3D12RootSignature> GetRootSignature() const;
	UINT GetSize() const;
	const std::vector<CD3DX12_ROOT_PARAMETER>& GetRootParameters() const;
};

struct Constants
{
	void* constants;
	UINT numConstants = 0;
	UINT numOffset = 0;
};

using BindResource = std::variant<D3D12_GPU_DESCRIPTOR_HANDLE, D3D12_GPU_VIRTUAL_ADDRESS, Constants>;

class ResourceSet
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const std::shared_ptr<RootSignature> pRootSignature_;
	std::vector<DescriptorManager> descriptorManagers_;
	std::vector<BindResource> bindedResources_;

public:
	ResourceSet(std::shared_ptr<RootSignature> pRootSignature, std::initializer_list<std::variant<DescriptorManager, std::shared_ptr<Buffer>, Constants>> bindedResources);
	~ResourceSet() = default;

	std::shared_ptr<RootSignature> GetRootSignature() const;
	const std::vector<BindResource> GetBindedResources() const;
	const std::vector<DescriptorManager> GetDescManagers() const;
};