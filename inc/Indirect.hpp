#pragma once

#include "pch.hpp"

#include "Alias.hpp"

namespace sqrp
{
	class Device;
	class RootSignature;

	enum class IndirectType
	{
		Draw, DrawIndexed, Dispatch, VB, IB, Constant, CBV, SRV, UAV, Ray, Mesh, IncConstant
	};

	struct IndirectDesc
	{
		IndirectType type_ = IndirectType::Draw;
		UINT rootParameterIndex_ = 0;
		UINT numConstant_ = 0;
	};

	class Indirect
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const Device* pDevice_ = nullptr;
		std::wstring name_;
		ComPtr<ID3D12CommandSignature> cmdSig_ = nullptr;

	public:
		Indirect(
			const Device& device,
			std::wstring name,
			std::initializer_list<IndirectDesc> indirectDescs,
			UINT byteStride,
			const RootSignatureHandle& rootSignature = nullptr
		);
		Indirect(const Indirect&) = delete;
		Indirect& operator=(const Indirect&) = delete;
		const ComPtr<ID3D12CommandSignature>& GetCommandSignature() const;
	};
}