#pragma once

#include "pch.hpp"

#include "Alias.hpp"

#include "Dxc.hpp"

namespace sqrp
{
	enum class CompilerType
	{
		Dxc,
		glslang
	};
	class ThreadPool;

	class Compiler
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;
		std::vector<std::unique_ptr<Dxc>> workerDxcs_;
		std::unique_ptr<ThreadPool> compileTP_ = nullptr;
		CompilerType compilerType_ = CompilerType::Dxc;

	public:
		Compiler() = delete;
		Compiler(CompilerType type, int numThreads);
		~Compiler();
		void CreateShader(ShaderHandle& shader, ShaderType shaderType, const std::wstring& fileName, const std::wstring& entry, std::vector<std::wstring> additionalOption = {}, const std::wstring& includePath = L"");
		void WaitFinishCompile();
		
	};
}