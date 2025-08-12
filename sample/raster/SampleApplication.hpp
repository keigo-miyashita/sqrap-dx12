#pragma once

#include <pch.hpp>
#include <sqrap.hpp>

#include "SampleScene.hpp"

class SampleApplication : sqrp::Application
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	SampleScene sampleScene_;

public:
	SampleApplication(std::string windowName, unsigned int window_width = 1280, unsigned int window_height = 720);
	~SampleApplication() = default;
	virtual bool Init() override;
	virtual void Run() override;
	virtual void Terminate() override;
};