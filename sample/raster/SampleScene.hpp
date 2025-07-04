#pragma once

#include <pch.hpp>
#include <sqrap.hpp>

struct CameraMatrix
{
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX proj;
};

struct Light
{
	DirectX::XMFLOAT4 pos;
	DirectX::XMFLOAT4 color;
};

struct Object
{
	DirectX::XMMATRIX model;
	DirectX::XMMATRIX invTransModel;
};

struct Color
{
	DirectX::XMFLOAT4 color;
};

class SampleScene
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	Device device_;
	DXC dxc_;
	CommandHandle command_;
	SwapChainHandle swapChain_;
	MeshHandle sphere_;
	// GUI
	GUIHandle GUI_;

	// Scene Items
	Camera camera_;
	Light light0_;
	Object sphere0_;

	// Resources
	BufferHandle cameraBuffer_;
	BufferHandle light0Buffer_;
	BufferHandle sphere0Buffer_;

	// Shaders
	ShaderHandle simpleVS_;
	ShaderHandle lambertPS_;

	// Descriptor
	DescriptorManagerHandle sphere0DescManager_;

	// RootSignature
	RootSignatureHandle sphere0RootSignature_;

	// Pipeline
	GraphicsPipelineHandle lambert_;

	ConstantsHandle ColorConstants_;
	Color diffuseColor_;

	void BeginRender();
	void EndRender();

public:
	SampleScene();
	~SampleScene() = default;
	bool Init(const Application& app);
	void Render();
};