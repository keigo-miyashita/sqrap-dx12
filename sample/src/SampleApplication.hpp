#pragma once

#include <common.hpp>

#include "SampleScene.hpp"

class SampleApplication : Application
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	SampleScene sampleScene_;

public:
	SampleApplication(std::string windowName, unsigned int window_width = 1280, unsigned int window_height = 720);
	~SampleApplication() = default;
	/*virtual int Input(UINT msg, WPARAM wparam, LPARAM lparam) override;*/
	virtual bool Init() override;
	bool Init(ComPtr<ID3D12DebugDevice>& debugDevice);
	virtual void Run() override;
	virtual void Terminate() override;
};