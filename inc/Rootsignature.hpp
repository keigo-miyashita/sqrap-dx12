#pragma once

#include "pch.hpp"

#include "Descriptormanager.hpp"
#include "Resource.hpp"

#include "Alias.hpp"

namespace sqrp
{
	class DescriptorManager;
	class Device;

	enum class RootParamType
	{
		DescTable, CBV, SRV, UAV, Constant,
	};

	struct DirectRootParamDesc
	{
		// Constant‚âDescriptor‚ð’¼Ú“o˜^‚·‚é‚Æ‚«‰º‚Ì1‚Â
		UINT numReg_ = 0;
		// 32BitsConstant—p
		// 1‚Â32Bits = 4Bytes
		// —ájfloat4 -> 16 Bytes -> numConstant_ = 4
		UINT numConstant_ = 0;
	};

	struct RootParameter
	{
		RootParamType rootParamType_;
		std::variant<DescriptorManagerHandle, DirectRootParamDesc> rootParamDesc_;
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
		std::vector<CD3DX12_ROOT_PARAMETER1> rps_;
		std::wstring name_;
		UINT size_ = 0;

	public:
		RootSignature(const Device& device, D3D12_ROOT_SIGNATURE_FLAGS flag, std::initializer_list<RootParameter> rootParams, std::wstring name = L"");
		~RootSignature() = default;
		ComPtr<ID3D12RootSignature> GetRootSignature() const;
		UINT GetSize() const;
		const std::vector<CD3DX12_ROOT_PARAMETER1>& GetRootParameters() const;
	};

	//struct Constants
	//{
	//	void* constants_;
	//	UINT numConstants_ = 0;
	//	UINT numOffset_ = 0;
	//};

	//using BindResource = std::variant<D3D12_GPU_DESCRIPTOR_HANDLE, D3D12_GPU_VIRTUAL_ADDRESS, ConstantsHandle>;

	struct ResourceSetDesc
	{
		std::variant<DescriptorManagerHandle, BufferHandle, ConstantsHandle> bindResource;
	};

	class ResourceSet
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const RootSignatureHandle rootSignature_;
		std::vector<ResourceSetDesc> resourceSetDescs_;

	public:
		ResourceSet(RootSignatureHandle rootSignature, std::vector<ResourceSetDesc> resourceSetDescs);
		~ResourceSet() = default;

		RootSignatureHandle GetRootSignature() const;
		const std::vector<ResourceSetDesc>& GetResourceSetDescs() const;
	};
}