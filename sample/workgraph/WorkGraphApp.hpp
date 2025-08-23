#pragma once

#include <pch.hpp>
#include <sqrap.hpp>

#include "WorkGraphScene.hpp"

class WorkGraphApp : public sqrp::Application
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	WorkGraphScene sampleScene_;

public:
	WorkGraphApp(std::string windowName, unsigned int window_width = 1280, unsigned int window_height = 720);
	~WorkGraphApp() = default;
	virtual bool InitMember() override;
	virtual void Render() override;
	virtual void Terminate() override;
};