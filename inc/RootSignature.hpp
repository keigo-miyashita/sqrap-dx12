#pragma once

#include "pch.hpp"

#include "DescriptorLayout.hpp"
#include "DescriptorManager.hpp"
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
		// ConstantやDescriptorを直接登録するとき下の1つ
		UINT numReg_ = 0;
		// 32BitsConstant用
		// 1つ32Bits = 4Bytes
		// 例）float4 -> 16 Bytes -> numConstant_ = 4
		UINT numConstant_ = 0;
	};

	struct SetByIndirect
	{
	};

	using RootArguments = std::variant<DescriptorManagerHandle, BufferHandle, ConstantsHandle, SetByIndirect>;
	using LocalRootArguments = std::variant<DescriptorManagerHandle, BufferHandle, ConstantsHandle>;

	struct RootParameter
	{
		RootParamType rootParamType_ = RootParamType::DescTable;
		std::variant<DirectRootParamDesc, DescriptorLayout> rootParamDesc_;
		D3D12_SHADER_VISIBILITY shaderVisibility_ = D3D12_SHADER_VISIBILITY_ALL;
	};

	class RootSignature
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		ComPtr<ID3D12RootSignature> rootSig_ = nullptr;
		D3D12_ROOT_SIGNATURE_FLAGS flag_ = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		std::vector<CD3DX12_ROOT_PARAMETER1> rps_;
		std::vector<RootParameter> rootParams_;
		std::wstring name_;
		UINT size_ = 0;
		std::set<std::pair<ViewType, UINT>> usedRegs_;

	public:
		static UINT AlignRootArgument(UINT size);
		template<typename T>
		void ValidateRootArgument(UINT index, const T& rootArgument) const
		{
			if (index >= rootParams_.size())
			{
				throw std::runtime_error("[RootSignature::ValidateRootArgument] index " + std::to_string(index) + " is out of range of root parameters");
			}
			switch (rootParams_[index].rootParamType_)
			{
			case RootParamType::DescTable:
			if (!std::holds_alternative<DescriptorManagerHandle>(rootArgument))
			{
				throw std::runtime_error("[RootSignature::ValidateRootArgument] expected a DescriptorManagerHandle for root parameter " + std::to_string(index));
			}
			break;
			case RootParamType::CBV:
			case RootParamType::SRV:
			case RootParamType::UAV:
				if (!std::holds_alternative<BufferHandle>(rootArgument))
				{
					throw std::runtime_error("[RootSignature::ValidateRootArgument] expected a BufferHandle for root parameter " + std::to_string(index));
				}
				break;
			case RootParamType::Constant:
				if (!std::holds_alternative<ConstantsHandle>(rootArgument))
				{
					throw std::runtime_error("[RootSignature::ValidateRootArgument] expected a ConstantsHandle for root parameter " + std::to_string(index));
				}
				if (std::get<ConstantsHandle>(rootArgument)->GetNumConstants() != std::get<DirectRootParamDesc>(rootParams_[index].rootParamDesc_).numConstant_) {
					throw std::runtime_error("[RootSignature::ValidateRootArgument] the number of constants does not match numConstant_ of root parameter " + std::to_string(index));
				}
				break;
			}
		}
		RootSignature(const Device& device, std::wstring name, D3D12_ROOT_SIGNATURE_FLAGS flag, std::initializer_list<RootParameter> rootParams);
		RootSignature(const RootSignature&) = delete;
		RootSignature& operator=(const RootSignature&) = delete;
		const ComPtr<ID3D12RootSignature>& GetRootSignature() const;
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
		ResourceSet(const RootSignatureHandle& rootSignature, const std::vector<ResourceSetDesc>& resourceSetDescs);
		ResourceSet(const ResourceSet&) = delete;
		ResourceSet& operator=(const ResourceSet&) = delete;

		RootSignatureHandle GetRootSignature() const;
		const std::vector<ResourceSetDesc>& GetResourceSetDescs() const;
	};
}
