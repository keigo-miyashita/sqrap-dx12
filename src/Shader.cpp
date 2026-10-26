#include "Shader.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	Shader::Shader(const Dxc& dxc, ShaderType shaderType, const wstring& fileName, const wstring& entry, const vector<wstring>& additionalOption, const wstring& includePath)
		: shaderType_(shaderType), entryName_(entry)
	{
		dxc.CompileShader(blob_, shaderType_, fileName, entry, additionalOption, includePath);
	}

	// コンパイル済みのバイトコードから作る。DXCは呼ばない
	Shader::Shader(ShaderType shaderType, ComPtr<IDxcBlob> blob, const wstring& entry)
		: shaderType_(shaderType), blob_(blob), entryName_(entry)
	{
	}

	const ComPtr<IDxcBlob>& Shader::GetBlob() const
	{
		return blob_;
	}

	const wstring& Shader::GetEntryName() const
	{
		return entryName_;
	}
}