#pragma once

#include <pch.hpp>

namespace sqrp
{
	class Shader;

	enum class ShaderType {
		Vertex, Pixel, Geometry, Domain, Hull, Amplification, Mesh, Compute, Raytracing, WorkGraph, Library
	};

	class Dxc
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		ComPtr<IDxcCompiler3> compiler_ = nullptr;
		ComPtr<IDxcLibrary> library_ = nullptr;
		ComPtr<IDxcUtils> utils_ = nullptr;

		void InitializeDxc();

	public:
		Dxc() = default;
		Dxc(const Dxc&) = delete;
		Dxc& operator=(const Dxc&) = delete;
		void Init();
		void CompileShader(ComPtr<IDxcBlob>& shaderBlob, ShaderType shaderType, const std::wstring& fileName, const std::wstring& entry, std::vector<std::wstring> additionalOption = {}, const std::wstring& includePath = L"") const;
		std::shared_ptr<Shader> CreateShader(ShaderType shaderType, const std::wstring& fileName, const std::wstring& entry, std::vector<std::wstring> additionalOption = {}, const std::wstring& includePath = L"") const;
		const ComPtr<IDxcCompiler3>& GetCompiler() const;
		const ComPtr<IDxcLibrary>& GetLibrary() const;
		const ComPtr<IDxcUtils>& GetUtils() const;
	};
}