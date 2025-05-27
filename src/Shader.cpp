#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

Shader::Shader(const DXC& dxc, ShaderType::Type shaderType, const std::wstring& fileName, const std::wstring& entry, const std::wstring& includePath)
	: pDxc_(&dxc), shaderType_(shaderType)
{
	pDxc_->CompileShader(blob_, shaderType_, fileName, entry, includePath);
}

ComPtr<IDxcBlob> Shader::GetBlob() const
{
	return blob_;
}