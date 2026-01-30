#include "Shader.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	Shader::Shader(const DXC& dxc, ShaderType shaderType, const std::wstring& fileName, const std::wstring& entry, std::vector<const wchar_t*> additionalOption, const std::wstring& includePath)
		: pDxc_(&dxc), shaderType_(shaderType), entryName_(entry)
	{
		pDxc_->CompileShader(blob_, shaderType_, fileName, entry, additionalOption, includePath);
	}

	ComPtr<IDxcBlob> Shader::GetBlob() const
	{
		return blob_;
	}

	std::wstring Shader::GetEntryName() const
	{
		return entryName_;
	}
}