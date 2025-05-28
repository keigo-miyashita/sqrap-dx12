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
	/*const DescriptorManager& descriptorManager_;
	DirectRootParamDesc directDesc_;*/
	D3D12_SHADER_VISIBILITY shaderVisibility_ = D3D12_SHADER_VISIBILITY_ALL;
};

//class RootParameter
//{
//private:
//	RootParamType rootParamType_;
//	// Descriptor table‚ð“o˜^‚·‚é‚Æ‚«‰º‚Ì1‚Â
//	std::variant<const DescriptorManager, DirectRootParamDesc> rootParamDesc_;
//	/*const DescriptorManager& descriptorManager_;
//	DirectRootParamDesc directDesc_;*/
//	D3D12_SHADER_VISIBILITY shaderVisibility_ = D3D12_SHADER_VISIBILITY_ALL;
//
//public:
//	RootParameter();
//	~RootParameter() = default;
//	void InitAsDescriptorTable(UINT descTableSize, const CD3DX12_DESCRIPTOR_RANGE* pDescTable, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL);
//	void InitAsConstantBufferView(UINT numReg, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL);
//	void InitAsShaderResourceView(UINT numReg, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL);
//	void InitAsUnorderedAccessView(UINT numReg, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL);
//	void InitAs32BitConstant(UINT numReg, UINT numConstant = 1);
//	D3D12_ROOT_PARAMETER_TYPE GetRootParamType();
//	UINT GetDescTableSize();
//	const CD3DX12_DESCRIPTOR_RANGE* GetPDescTable() const;
//	UINT GetNumReg();
//	D3D12_SHADER_VISIBILITY GetShaderVisibility();
//	UINT GetNumConstant();
//	
//};

class RootSignature
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const Device* pDevice_ = nullptr;
	ComPtr<ID3D12RootSignature> rootSig_ = nullptr;
	D3D12_ROOT_SIGNATURE_FLAGS flag_ = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	//std::vector<RootParameter> rootParams_;
	std::wstring name_;

public:
	RootSignature(const Device& device, D3D12_ROOT_SIGNATURE_FLAGS flag, std::initializer_list<RootParameter> rootParams, std::wstring name = L"");
	~RootSignature() = default;
	/*bool Init(Device* pDevice);
	bool InitializeRootSignature(D3D12_ROOT_SIGNATURE_FLAGS flags, std::wstring name = L"RootSignature");
	void AddDescriptorTable(const DescriptorManager& descriptorManager, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL);
	void AddDescriptor(D3D12_ROOT_PARAMETER_TYPE rootParamType, UINT numReg, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL);
	void AddConstant(UINT numReg, UINT numConstant = 1);*/
	ComPtr<ID3D12RootSignature> GetRootSignature() const;
};