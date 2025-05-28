#pragma once

#include <common.hpp>

class DescriptorManager;
class Device;

enum class RootParamType
{
	DescTable,
	CBV,
	SRV,
	UAV,
	Constant,
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
	std::wstring name_;

public:
	RootSignature(const Device& device, D3D12_ROOT_SIGNATURE_FLAGS flag, std::initializer_list<RootParameter> rootParams, std::wstring name = L"");
	~RootSignature() = default;
	ComPtr<ID3D12RootSignature> GetRootSignature() const;
};