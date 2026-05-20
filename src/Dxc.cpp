#include "Dxc.hpp"

#include "Device.hpp"
#include "Shader.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	void DXC::InitializeDxc()
	{
		auto result = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(compiler_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("Failed to DxcCreateInstance for compiler_ : " + to_string(result));
		}
		result = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(library_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("Failed to DxcCreateInstance for library_ : " + to_string(result));
		}
		result = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("Failed to DxcCreateInstance for utils_ : " + to_string(result));
		}
	}

	void DXC::Init()
	{
		InitializeDxc();
	}

	void DXC::CompileShader(ComPtr<IDxcBlob>& shaderBlob, ShaderType shaderType, const wstring& fileName, const wstring& entry, vector<const wchar_t*> additionalOption, const wstring& includePath) const
	{
		ComPtr<IDxcIncludeHandler> incHandler = nullptr;
		ComPtr<IDxcBlob> incBlob = nullptr;

		HRESULT result = utils_->CreateDefaultIncludeHandler(incHandler.ReleaseAndGetAddressOf());
		if (FAILED(result)) {
			throw runtime_error("Failed to CreateDefaultIncludeHandler : " + to_string(result));
		}

		if (!includePath.empty()) {
			DWORD attrib = GetFileAttributesW(includePath.c_str());
			if (attrib == INVALID_FILE_ATTRIBUTES) {
				throw runtime_error("Failed to get include file : ");
			}

			result = incHandler->LoadSource(includePath.c_str(), incBlob.ReleaseAndGetAddressOf());
			if (FAILED(result)) {
				throw runtime_error("Failed to LoadSource : " + to_string(result));
			}
		}

		vector<const wchar_t*> options;
		if (!additionalOption.empty()) {
			for (int i = 0; i < additionalOption.size(); i++) {
				options.push_back(additionalOption[i]);
			}
		}
#ifdef _DEBUG
		options.push_back(L"-Zi");
		options.push_back(L"-Fd");
		wstring erasedExtension = fileName;
		size_t pos = erasedExtension.find(L'.');
		if (pos != wstring::npos) {
			erasedExtension.erase(pos);
		}
		options.push_back((erasedExtension + L".pdb").c_str());
		cout << "Generate pdb files" << endl;
#endif // DEBUG
		if (shaderType == ShaderType::WorkGraph) {
			options.push_back(L"-select-validator internal");
		}
		ComPtr<IDxcBlobEncoding> source = nullptr;
		result = library_->CreateBlobFromFile(fileName.c_str(), nullptr, source.ReleaseAndGetAddressOf());
		if (FAILED(result)) {
			throw runtime_error("Failed to CreateBlobFromFile : " + to_string(result));
		}

		ComPtr<IDxcOperationResult> dxcResult = nullptr;
		if (!entry.empty()) {
			result = compiler_->Compile(source.Get(), fileName.c_str(), entry.c_str(), ShaderModel[ToInt(shaderType)].c_str(), options.data(), options.size(), nullptr, 0, incHandler.Get(), dxcResult.ReleaseAndGetAddressOf());
		}
		else {
			result = compiler_->Compile(source.Get(), fileName.c_str(), nullptr, ShaderModel[ToInt(shaderType)].c_str(), options.data(), options.size(), nullptr, 0, incHandler.Get(), dxcResult.ReleaseAndGetAddressOf());
		}
		ComPtr<IDxcBlobEncoding> err;
		if (FAILED(result)) {
			dxcResult->GetErrorBuffer(err.ReleaseAndGetAddressOf());
			if (err) {
				DebugOutputFormatString((char*)err->GetBufferPointer());
			}
			throw runtime_error("Failed to Compile : " + to_string(result));
		}

		HRESULT tmpResult;
		dxcResult->GetStatus(&tmpResult);
		if (FAILED(tmpResult)) {
			dxcResult->GetErrorBuffer(err.ReleaseAndGetAddressOf());
			if (err) {
				DebugOutputFormatString((char*)err->GetBufferPointer());
			}
			throw runtime_error("Failed to GetResult : " + to_string(result));
		}
		dxcResult->GetResult(shaderBlob.ReleaseAndGetAddressOf());
	}

	shared_ptr<Shader> DXC::CreateShader(ShaderType shaderType, const wstring& fileName, const wstring& entry, vector<const wchar_t*> additionalOption, const wstring& includePath) const
	{
		return make_shared<Shader>(*this, shaderType, fileName, entry, additionalOption, includePath);
	}

	ComPtr<IDxcCompiler> DXC::GetCompiler() const
	{
		return compiler_;
	}

	ComPtr<IDxcLibrary> DXC::GetLibrary() const
	{
		return library_;
	}

	ComPtr<IDxcUtils> DXC::GetUtils() const
	{
		return utils_;
	}
}