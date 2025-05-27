#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

bool DXC::InitializeDxc()
{
	auto result = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(compiler_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		return false;
	}
	result = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(library_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		return false;
	}
	result = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		return false;
	}

	return true;
}

DXC::DXC()
{

}

bool DXC::Init()
{
	if (!InitializeDxc()) {
		return false;
	}

	return true;
}

bool DXC::CompileShader(ComPtr<IDxcBlob>& shaderBlob, ShaderType::Type shaderType, const std::wstring& fileName, const std::wstring& entry, const std::wstring& includePath) const
{
	ComPtr<IDxcIncludeHandler> incHandler_ = nullptr;
	ComPtr<IDxcBlob> incBlob_ = nullptr;

	auto result = utils_->CreateDefaultIncludeHandler(incHandler_.ReleaseAndGetAddressOf());
	if (FAILED(result)) {
		return false;
	}

	wcout << includePath << endl;
	if (!includePath.empty()) {
		wcout << L"has include path" << endl;
		DWORD attrib = GetFileAttributesW(includePath.c_str());
		if (attrib == INVALID_FILE_ATTRIBUTES) {
			cerr << "include hlsl not found\n";
			return false;
		}

		result = incHandler_->LoadSource(includePath.c_str(), incBlob_.ReleaseAndGetAddressOf());
		if (FAILED(result)) {
			return false;
		}
	}

	vector<const wchar_t*> options;
#ifdef _DEBUG
	options.push_back(L"-Zi");
	options.push_back(L"-Fd");
	wstring erasedExtension = fileName;
	size_t pos = erasedExtension.find(L'.');
	if (pos != std::wstring::npos) {
		erasedExtension.erase(pos);
	}
	options.push_back((erasedExtension + L".pdb").c_str());
	cout << "Generate pdb files" << endl;
#endif // DEBUG
	if (shaderType == ShaderType::WorkGraph) {
		options.push_back(L"-select-validator internal");
	}
	ComPtr<IDxcBlobEncoding> source = nullptr;
	if (FAILED(library_->CreateBlobFromFile(fileName.c_str(), nullptr, source.ReleaseAndGetAddressOf()))) {
		return false;
	}

	ComPtr<IDxcOperationResult> dxcResult = nullptr;
	if (!entry.empty()) {
		result = compiler_->Compile(source.Get(), fileName.c_str(), entry.c_str(), ShaderModel[shaderType].c_str(), options.data(), options.size(), nullptr, 0, incHandler_.Get(), dxcResult.ReleaseAndGetAddressOf());
	}
	else {
		result = compiler_->Compile(source.Get(), fileName.c_str(), nullptr, ShaderModel[shaderType].c_str(), options.data(), options.size(), nullptr, 0, incHandler_.Get(), dxcResult.ReleaseAndGetAddressOf());
	}
	ComPtr<IDxcBlobEncoding> err;
	if (FAILED(result)) {
		dxcResult->GetErrorBuffer(err.ReleaseAndGetAddressOf());
		if (err) {
			DebugOutputFormatString((char*)err->GetBufferPointer());
		}
		return false;
	}

	HRESULT tmpResult;
	dxcResult->GetStatus(&tmpResult);
	if (SUCCEEDED(tmpResult)) {
		dxcResult->GetResult(shaderBlob.ReleaseAndGetAddressOf());
		return true;
	}
	dxcResult->GetErrorBuffer(err.ReleaseAndGetAddressOf());
	if (err) {
		DebugOutputFormatString((char*)err->GetBufferPointer());
	}
	return false;

}

std::shared_ptr<Shader> DXC::CreateShader(ShaderType::Type shaderType, const std::wstring& fileName, const std::wstring& entry, const std::wstring& includePath)
{
	return make_shared<Shader>(*this, shaderType, fileName, entry, includePath);
}

ComPtr<IDxcCompiler> DXC::GetCompiler()
{
	return compiler_;
}

ComPtr<IDxcLibrary> DXC::GetLibarary()
{
	return library_;
}

ComPtr<IDxcUtils> DXC::GetUtils()
{
	return utils_;
}