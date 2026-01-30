#pragma once

#include "pch.hpp"

#include "Dxc.hpp"

namespace sqrp
{
	class DXC;

	class Shader
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const DXC* pDxc_ = nullptr;
		ShaderType shaderType_;
		// Shader‚ÌByte code‚ðŠi”[
		ComPtr<IDxcBlob> blob_ = nullptr;
		std::wstring entryName_;

	public:
		Shader(const DXC& dxc, ShaderType shaderType, const std::wstring& fileName, const std::wstring& entry, std::vector<const wchar_t*> additionalOption = {}, const std::wstring& includePath = L"");
		~Shader() = default;
		ComPtr<IDxcBlob> GetBlob() const;
		std::wstring GetEntryName() const;
	};
}