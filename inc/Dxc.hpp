#pragma once

#include <common.hpp>

struct ShaderType{
	enum Type {
		Vertex, Pixel, Geometry, Domain, Hull, Amplification, Mesh, Compute, WorkGraph, Library
	};
};

class DXC
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	// D3D12 descriptor heap
	ComPtr<IDxcCompiler> compiler_ = nullptr;
	ComPtr<IDxcLibrary> library_ = nullptr;
	ComPtr<IDxcUtils> utils_ = nullptr;

	bool InitializeDxc();


public:
	DXC();
	~DXC() = default;
	bool Init();
	bool CompileShader(ComPtr<IDxcBlob>& shaderBlob, const LPCWSTR& includePath, LPCWSTR fileName, ShaderType::Type shaderType, LPCWSTR model, LPCWSTR entry = L"main", std::vector<std::wstring> additionalOption = {}) const;
	bool CompileShader(ComPtr<IDxcBlob>& shaderBlob, std::wstring fileName, ShaderType::Type shaderType, LPCWSTR model, LPCWSTR entry = L"main", std::vector<std::wstring> additionalOption = {}) const;
	ComPtr<IDxcCompiler> GetCompiler();
	ComPtr<IDxcLibrary> GetLibarary();
	ComPtr<IDxcUtils> GetUtils();
	ComPtr<IDxcIncludeHandler> GetIncHandler();
	ComPtr<IDxcBlob> GetIncBlob();
};