#pragma once

#include "pch.hpp"

#include "Dxc.hpp"

namespace sqrp
{
	class Shader
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const DXC* pDxc_ = nullptr;
		ShaderType shaderType_;
		// Shader��Byte code��i�[
		ComPtr<IDxcBlob> blob_ = nullptr;
		std::wstring entryName_;

	public:
		Shader(const DXC& dxc, ShaderType shaderType, const std::wstring& fileName, const std::wstring& entry, const std::vector<const wchar_t*>& additionalOption = {}, const std::wstring& includePath = L"");
		// コンパイル済みのバイトコードから作る。呼び出し側でDXILをディスクに
		// キャッシュしておき、ソースが変わっていなければ読み込むために使う。
		// HLSL->DXILのコンパイルは巨大なヘッダをエントリごとに舐め直すので、
		// エントリ数が多いと起動が数十秒になる(実測: 35本で約70秒)。
		// ソースが同じなら結果は毎回同じなのでキャッシュが素直に効く
		Shader(const DXC& dxc, ShaderType shaderType, ComPtr<IDxcBlob> blob, const std::wstring& entry);
		~Shader() = default;
		ComPtr<IDxcBlob> GetBlob() const;
		const std::wstring& GetEntryName() const;
	};
}