#include "Compiler.hpp"
#include "Dxc.hpp"
#include "Shader.hpp"
#include "ThreadPool.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	Compiler::Compiler(CompilerType type, int numThreads) : compilerType_(type)
	{
		if (type != CompilerType::Dxc) {
			throw runtime_error("[Compiler::Compiler] unsupported compiler type");
		}
		workerDxcs_.reserve(numThreads);
		for (int i = 0; i < numThreads; i++) {
			workerDxcs_.emplace_back(make_unique<Dxc>());
			workerDxcs_[i]->Init();
		}
		compileTP_ = make_unique<ThreadPool>(numThreads);
	}

	Compiler::~Compiler()
	{
		compileTP_->Abort();
	}

	void Compiler::CreateShader(ShaderHandle& shader, ShaderType shaderType, const std::wstring& fileName, const std::wstring& entry, std::vector<std::wstring> additionalOption, const std::wstring& includePath)
	{
		auto task = [this, &shader, shaderType, fileName, entry, additionalOption, includePath]()
			{
				shader = workerDxcs_[ThreadPool::CurrentWorkerIndex()]->CreateShader(shaderType, fileName, entry, additionalOption, includePath);

			};
		compileTP_->Push(task);
	}

	void Compiler::WaitFinishCompile()
	{
		compileTP_->WaitUntilIdle();
	}
}