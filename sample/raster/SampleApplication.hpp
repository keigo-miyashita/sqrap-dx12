#pragma once

#include <pch.hpp>
#include <sqrap.hpp>

struct Light
{
	DirectX::XMFLOAT4 pos;
	DirectX::XMFLOAT4 color;
};

struct Color
{
	DirectX::XMFLOAT4 color;
};

class SampleApplication : public sqrp::Application
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	//SampleScene sampleScene_;
	sqrp::Device device_;
	sqrp::DXC dxc_;
	sqrp::CommandHandle command_;
	sqrp::SwapChainHandle swapChain_;
	sqrp::MeshHandle sphere_;
	// GUI
	sqrp::GUIHandle GUI_;

	// Scene Items
	sqrp::Camera camera_;
	Light light0_;
	sqrp::TransformMatrix sphere0_;

	// Resources
	sqrp::BufferHandle cameraBuffer_;
	sqrp::BufferHandle light0Buffer_;
	sqrp::BufferHandle sphere0Buffer_;

	// Shaders
	sqrp::ShaderHandle simpleVS_;
	sqrp::ShaderHandle lambertPS_;

	// Descriptor
	sqrp::DescriptorManagerHandle sphere0DescManager_;

	// RootSignature
	sqrp::RootSignatureHandle sphere0RootSignature_;

	// Pipeline
	sqrp::GraphicsPipelineHandle lambert_;

	sqrp::ConstantsHandle ColorConstants_;
	Color diffuseColor_;

public:
	SampleApplication(std::string windowName, unsigned int window_width = 1280, unsigned int window_height = 720);
	~SampleApplication() = default;
	virtual bool OnStart() override;
	virtual void OnUpdate() override;
	virtual void OnTerminate() override;
};