#pragma once

#include <common.hpp>

class Shader
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ShaderType::Type shaderType_;
	// Shader‚ÌByte code‚ðŠi”[
	ComPtr<IDxcBlob> blob_ = nullptr;

	bool CreateShader(const DXC& dxc, std::wstring fileName, ShaderType::Type shaderType, LPCWSTR entry = L"main");

public:
	Shader();
	~Shader() = default;
	bool Init(const DXC& dxc, std::wstring fileName, ShaderType::Type shaderType, LPCWSTR entry = L"main");
	ComPtr<IDxcBlob> GetBlob() const;
};