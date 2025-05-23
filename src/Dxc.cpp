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

bool DXC::CompileShader(ComPtr<IDxcBlob>& shaderBlob, const LPCWSTR& includePath, LPCWSTR fileName, ShaderType::Type shaderType, LPCWSTR model, LPCWSTR entry, std::vector<std::wstring> additionalOption) const
{
	ComPtr<IDxcIncludeHandler> incHandler_ = nullptr;
	ComPtr<IDxcBlob> incBlob_ = nullptr;

	auto result = utils_->CreateDefaultIncludeHandler(incHandler_.ReleaseAndGetAddressOf());
	if (FAILED(result)) {
		return false;
	}

	DWORD attrib = GetFileAttributesW(includePath);
	if (attrib == INVALID_FILE_ATTRIBUTES) {
		cerr << "include hlsl not found\n";
		return false;
	}

	result = incHandler_->LoadSource(includePath, incBlob_.ReleaseAndGetAddressOf());
	if (FAILED(result)) {
		return false;
	}

	vector<const wchar_t*> options;
#ifdef DEBUG
	options.push_back(L"-Zi");
	options.push_back(L"-Fd");
	options.push_back(fileName + L".pdb");
#endif // DEBUG
	if (additionalOption.size() != 0) {
		for (int i = 0; i < additionalOption.size(); i++) {
			options.push_back(additionalOption[i].c_str());
		}
	}
	ComPtr<IDxcBlobEncoding> source = nullptr;
	if (FAILED(library_->CreateBlobFromFile(fileName, nullptr, source.ReleaseAndGetAddressOf()))) {
		return false;
	}

	ComPtr<IDxcOperationResult> dxcResult = nullptr;
	result = compiler_->Compile(source.Get(), fileName, entry, model, options.data(), options.size(), nullptr, 0, incHandler_.Get(), dxcResult.ReleaseAndGetAddressOf());
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

bool DXC::CompileShader(ComPtr<IDxcBlob>& shaderBlob, wstring fileName, ShaderType::Type shaderType, LPCWSTR model, LPCWSTR entry, std::vector<std::wstring> additionalOption) const
{
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
	if (additionalOption.size() != 0) {
		for (int i = 0; i < additionalOption.size(); i++) {
			options.push_back(additionalOption[i].c_str());
		}
	}
	ComPtr<IDxcBlobEncoding> source = nullptr;
	if (FAILED(library_->CreateBlobFromFile(fileName.c_str(), nullptr, source.ReleaseAndGetAddressOf()))) {
		cerr << "Failed to create blob from file" << endl;
		return false;
	}

	ComPtr<IDxcOperationResult> dxcResult = nullptr;
	auto result = compiler_->Compile(source.Get(), fileName.c_str(), entry, model, options.data(), options.size(), nullptr, 0, nullptr, dxcResult.ReleaseAndGetAddressOf());
	ComPtr<IDxcBlobEncoding> err;
	if (FAILED(result)) {
		dxcResult->GetErrorBuffer(err.ReleaseAndGetAddressOf());
		if (err) {
			DebugOutputFormatString((char*)err->GetBufferPointer());
		}
		cerr << "Failed to Compile" << endl;
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