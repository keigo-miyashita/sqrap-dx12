#pragma once

//#include <common.hpp>

#include <pch.hpp>
//
//#include <Shader.hpp>

class Shader;

enum class ShaderType{
	Vertex, Pixel, Geometry, Domain, Hull, Amplification, Mesh, Compute, RayTracing, WorkGraph, Library
};

inline int ToInt(ShaderType type) {
	return static_cast<int>(type);
}

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

	ComPtr<IDxcCompiler> compiler_ = nullptr;
	ComPtr<IDxcLibrary> library_ = nullptr;
	ComPtr<IDxcUtils> utils_ = nullptr;

	bool InitializeDxc();

public:
	DXC();
	~DXC() = default;
	bool Init();
	bool CompileShader(ComPtr<IDxcBlob>& shaderBlob, ShaderType shaderType, const std::wstring& fileName, const std::wstring& entry, const std::wstring& includePath = L"") const;
	std::shared_ptr<Shader> CreateShader(ShaderType shaderType, const std::wstring& fileName, const std::wstring& entry, const std::wstring& includePath = L"");
	ComPtr<IDxcCompiler> GetCompiler();
	ComPtr<IDxcLibrary> GetLibarary();
	ComPtr<IDxcUtils> GetUtils();
	ComPtr<IDxcIncludeHandler> GetIncHandler();
	ComPtr<IDxcBlob> GetIncBlob();
};