#pragma once

#include <common.hpp>

struct ShaderType{
	enum Type {
		Vertex, Pixel, Geometry, Domain, Hull, Amplification, Mesh, Compute, RayTracing, WorkGraph, Library
	};
};

static std::wstring ShaderModel[] = {
	L"vs_6_6",
	L"ps_6_6",
	L"gs_6_6",
	L"ds_6_6",
	L"hs_6_6",
	L"as_6_6",
	L"ms_6_6",
	L"cs_6_8",
	L"lib_6_3",
	L"lib_6_9",
	L"lib_6_9"
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
	bool CompileShader(ComPtr<IDxcBlob>& shaderBlob, const LPCWSTR& includePath, LPCWSTR fileName, ShaderType::Type shaderType, LPCWSTR entry = L"main") const;
	bool CompileShader(ComPtr<IDxcBlob>& shaderBlob, std::wstring fileName, ShaderType::Type shaderType, LPCWSTR entry = L"main") const;
	ComPtr<IDxcCompiler> GetCompiler();
	ComPtr<IDxcLibrary> GetLibarary();
	ComPtr<IDxcUtils> GetUtils();
	ComPtr<IDxcIncludeHandler> GetIncHandler();
	ComPtr<IDxcBlob> GetIncBlob();
};