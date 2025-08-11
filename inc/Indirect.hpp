#pragma once

#include "pch.hpp"

#include "Alias.hpp"

namespace sqrp
{
	class Device;
	class RootSignature;

	enum class IndirectType
	{
		Draw, DrawIndexed, Dispatch, VB, IB, Constant, CBV, SRV, UAV, Ray, Mesh
	};

	struct IndirectDesc
	{
		IndirectType type_;
		UINT maxCommandCount_ = 0;
		BufferHandle argumentBuffer_ = nullptr;
		BufferHandle counterBuffer_ = nullptr;
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
		std::vector<D3D12_INDIRECT_ARGUMENT_DESC> indirectArgDesc_;
		D3D12_COMMAND_SIGNATURE_DESC cmdSigDesc_ = {};

	public:
		Indirect(const Device& device, std::initializer_list<IndirectDesc> indirectDescs, RootSignatureHandle rootSignature, UINT byteStride, std::wstring name = L"");
		~Indirect() = default;
		ComPtr<ID3D12CommandSignature> GetCommandSignature() const;
	};
}