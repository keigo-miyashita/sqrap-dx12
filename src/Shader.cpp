#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

bool Shader::CreateShader(const DXC& dxc, wstring fileName, ShaderType::Type shaderType, LPCWSTR model, LPCWSTR entry, std::vector<std::wstring> additionalOption)
{
	shaderType_ = shaderType;
	if (!dxc.CompileShader(blob_, fileName, shaderType, model, entry, additionalOption)) {
		cerr << "Failed to compile shader" << endl;
		return false;
	}
	pBinary_ = blob_->GetBufferPointer();
	binarySize = blob_->GetBufferSize();

	return true;
}

Shader::Shader()
{

}

bool Shader::Init(const DXC& dxc, wstring fileName, ShaderType::Type shaderType, LPCWSTR model, LPCWSTR entry, std::vector<std::wstring> additionalOption)
{
	if (!CreateShader(dxc, fileName, shaderType, model, entry, additionalOption)) {
		return false;
	}

	return true;
}

ComPtr<IDxcBlob> Shader::GetBlob() const
{
	return blob_;
}