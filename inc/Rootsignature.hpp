#pragma once

#include "pch.hpp"

#include "Descriptormanager.hpp"
#include "Resource.hpp"

#include "Alias.hpp"

namespace sqrp
{
	class Device;

	enum class RootParamType
	{
		DescTable, CBV, SRV, UAV, Constant,
	};

	struct DirectRootParamDesc
	{
		// Constant��Descriptor�𒼐ړo�^����Ƃ�����1��
		UINT numReg_ = 0;
		// 32BitsConstant�p
		// 1��32Bits = 4Bytes
		// ��jfloat4 -> 16 Bytes -> numConstant_ = 4
		UINT numConstant_ = 0;
		void* constantPtr_ = nullptr;
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
		std::vector<RootParameter> rootParams_;
		std::wstring name_;
		UINT size_ = 0;

	public:
		RootSignature(const Device& device, std::wstring name, D3D12_ROOT_SIGNATURE_FLAGS flag, std::initializer_list<RootParameter> rootParams);
		~RootSignature() = default;
		ComPtr<ID3D12RootSignature> GetRootSignature() const;
		UINT GetSize() const;
		const std::vector<CD3DX12_ROOT_PARAMETER1>& GetRootParameters() const;
		const std::vector<RootParameter>& GetRootParametersVec() const;
	};

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
		ResourceSet(RootSignatureHandle rootSignature, const std::vector<ResourceSetDesc>& resourceSetDescs);
		~ResourceSet() = default;

		RootSignatureHandle GetRootSignature() const;
		const std::vector<ResourceSetDesc>& GetResourceSetDescs() const;
	};
}