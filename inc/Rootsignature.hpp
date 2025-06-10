#pragma once

#include "pch.hpp"

#include "Descriptormanager.hpp"

#include "Alias.hpp"

class DescriptorManager;
class Device;

enum class RootParamType
{
	DescTable, CBV, SRV, UAV, Constant,
};

struct DescTableRootParamDesc
{
	DescriptorManagerHandle descManager;
};

struct DirectRootParamDesc
{
	// Constant��Descriptor�𒼐ړo�^����Ƃ�����1��
	UINT numReg = 0;
	// 32BitsConstant�p
	// 1��32Bits = 4Bytes
	// ��jfloat4 -> 16 Bytes -> numConstant_ = 4
	UINT numConstant = 0;
};

struct RootParameter
{
	RootParamType rootParamType_;
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

	const RootSignatureHandle rootSignature_;
	std::vector<DescriptorManagerHandle> descriptorManagers_;
	std::vector<BindResource> bindResources_;

public:
	ResourceSet(RootSignatureHandle rootSignature, std::initializer_list<std::variant<DescriptorManagerHandle, std::shared_ptr<Buffer>, Constants>> bindedResources);
	~ResourceSet() = default;

	RootSignatureHandle GetRootSignature() const;
	const std::vector<BindResource>& GetBindedResources() const;
	const std::vector<DescriptorManagerHandle>& GetDescManagers() const;
};