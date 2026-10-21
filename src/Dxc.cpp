#include "Dxc.hpp"

#include "Device.hpp"
#include "Shader.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	static wstring GetShaderModel(ShaderType shaderType)
	{
		switch (shaderType) {
			case ShaderType::Vertex:
				return L"vs_6_6";
			case ShaderType::Pixel:
				return L"ps_6_6";
			case ShaderType::Geometry:
				return L"gs_6_6";
			case ShaderType::Domain:
				return L"ds_6_6";
			case ShaderType::Hull:
				return L"hs_6_6";
			case ShaderType::Amplification:
				return L"as_6_6";
			case ShaderType::Mesh:
				return L"ms_6_6";
			case ShaderType::Compute:
				return L"cs_6_8";
			case ShaderType::Raytracing:
				return L"lib_6_3";
			case ShaderType::WorkGraph:
				return L"lib_6_9";
			case ShaderType::Library:
				return L"lib_6_9";
			default:
				throw runtime_error("[GetShaderModel] invalid shader type");
		}
	}

	void Dxc::InitializeDxc()
	{
		auto result = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(compiler_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[Dxc::InitializeDxc] DxcCreateInstance for compiler_ failed\nHRESULT = " + to_string(result));
		}
		result = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(library_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[Dxc::InitializeDxc] DxcCreateInstance for library_ failed\nHRESULT = " + to_string(result));
		}
		result = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[Dxc::InitializeDxc] DxcCreateInstance for utils_ failed\nHRESULT = " + to_string(result));
		}
	}

	void Dxc::Init()
	{
		InitializeDxc();
	}

	void Dxc::CompileShader(ComPtr<IDxcBlob>& shaderBlob, ShaderType shaderType, const wstring& fileName, const wstring& entry, vector<std::wstring> additionalOption, const wstring& includePath) const
	{
		ComPtr<IDxcIncludeHandler> incHandler = nullptr;
		ComPtr<IDxcBlob> incBlob = nullptr;

		HRESULT result = utils_->CreateDefaultIncludeHandler(incHandler.ReleaseAndGetAddressOf());
		if (FAILED(result)) {
			throw runtime_error("[Dxc::CompileShader] CreateDefaultIncludeHandler failed\nHRESULT = " + to_string(result));
		}

		if (!includePath.empty()) {
			DWORD attrib = GetFileAttributesW(includePath.c_str());
			if (attrib == INVALID_FILE_ATTRIBUTES) {
				throw runtime_error("[Dxc::CompileShader] include file not found");
			}

			result = incHandler->LoadSource(includePath.c_str(), incBlob.ReleaseAndGetAddressOf());
			if (FAILED(result)) {
				throw runtime_error("[Dxc::CompileShader] LoadSource failed\nHRESULT = " + to_string(result));
			}
		}

		vector<wstring> args;
		args.push_back(fileName);
		args.push_back(L"-T");
		args.push_back(GetShaderModel(shaderType));
		if (shaderType == ShaderType::Raytracing || shaderType == ShaderType::WorkGraph || shaderType == ShaderType::Library) {
			if (!entry.empty()) {
				cout << "[Dxc::CompileShader] entry is ignored for Raytracing, WorkGraph and Library shaders. Specify entry as export of state object." << endl;
			}
		}
		else if (!entry.empty()) {
			args.push_back(L"-E");
			args.push_back(entry);
		}
		for (const auto& opt : additionalOption) {
			args.push_back(opt);
		}

		// Extract only the file name from the full path
		wosyncstream wss(wcout);
		wstring shaderName = fileName.substr(fileName.find_last_of(L"/\\") + 1);
		wss << shaderName << L" [" << GetShaderModel(shaderType) << L"] " << entry;
#ifdef _DEBUG
		args.push_back(L"-Zi");
		//args.push_back(L"-Fd");
		args.push_back(L"-Qembed_debug");
		wss << L" generate gdb";
#endif // DEBUG
		wss << endl;

		if (shaderType == ShaderType::WorkGraph) {
			args.push_back(L"-select-validator");
			args.push_back(L"internal");
		}

		ComPtr<IDxcBlobEncoding> source = nullptr;
		result = utils_->LoadFile(fileName.c_str(), nullptr, source.ReleaseAndGetAddressOf());
		if (FAILED(result)) {
			throw runtime_error("[Dxc::CompileShader] LoadFile failed\nHRESULT = " + to_string(result));
		}
		BOOL knowEncoding = FALSE;
		UINT32 codePage = DXC_CP_ACP;
		source->GetEncoding(&knowEncoding, &codePage);

		DxcBuffer sourceBuffer = {};
		sourceBuffer.Ptr = source->GetBufferPointer();
		sourceBuffer.Size = source->GetBufferSize();
		sourceBuffer.Encoding = codePage;

		vector<const wchar_t*> argPtrs;
		argPtrs.reserve(args.size());
		for (const auto& arg : args) {
			argPtrs.push_back(arg.c_str());
		}

		ComPtr<IDxcResult> dxcResult = nullptr;
		result = compiler_->Compile(&sourceBuffer, argPtrs.data(), static_cast<UINT32>(argPtrs.size()), incHandler.Get(), IID_PPV_ARGS(dxcResult.ReleaseAndGetAddressOf()));

		if (FAILED(result)) {
			throw runtime_error("[Dxc::CompileShader] Compile failed\nHRESULT = " + to_string(result));
		}

		HRESULT status = S_OK;
		dxcResult->GetStatus(&status);
		if (FAILED(status)) {
			string errorMsg = "[Dxc::CompileShader] GetStatus failed\nHRESULT = " + to_string(status);
			ComPtr<IDxcBlobUtf8> errors = nullptr;
			dxcResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.ReleaseAndGetAddressOf()), nullptr);
			if (errors && errors->GetStringLength() > 0) {
				errorMsg += "\n" + string(errors->GetStringPointer(), errors->GetStringLength());
			}
			throw runtime_error(errorMsg);
		}
		dxcResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(shaderBlob.ReleaseAndGetAddressOf()), nullptr);
	}

	shared_ptr<Shader> Dxc::CreateShader(ShaderType shaderType, const wstring& fileName, const wstring& entry, vector<std::wstring> additionalOption, const wstring& includePath) const
	{
		return make_shared<Shader>(*this, shaderType, fileName, entry, additionalOption, includePath);
	}

	const ComPtr<IDxcCompiler3>& Dxc::GetCompiler() const
	{
		return compiler_;
	}

	const ComPtr<IDxcLibrary>& Dxc::GetLibrary() const
	{
		return library_;
	}

	const ComPtr<IDxcUtils>& Dxc::GetUtils() const
	{
		return utils_;
	}
}