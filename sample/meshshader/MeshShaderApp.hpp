#pragma once

#include <pch.hpp>
#include <sqrap.hpp>

#include "MeshShaderScene.hpp"

class MeshShaderApp : sqrp::Application
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	MeshShaderScene sampleScene_;

public:
	MeshShaderApp(std::string windowName, unsigned int window_width = 1280, unsigned int window_height = 720);
	~MeshShaderApp() = default;
	virtual bool Init() override;
	virtual void Run() override;
	virtual void Terminate() override;
};