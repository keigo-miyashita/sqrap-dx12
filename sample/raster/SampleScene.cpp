#include "SampleScene.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;
using namespace sqrp;

void SampleScene::BeginRender()
{
	auto bbIdx = swapChain_->GetSwapChain()->GetCurrentBackBufferIndex();

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(swapChain_->GetCurrentBackBuffer()->GetResource().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	command_->GetCommandList()->ResourceBarrier(1, &barrier);

	auto rtvHandle = swapChain_->GetRtvHeap()->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += bbIdx * device_.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	auto dsvHandle = swapChain_->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart();
	command_->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

	// Clear depth buffer
	command_->GetCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// Clear display
	float clearColor[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	command_->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// Set viewport and scissors
	auto viewPort = swapChain_->GetViewPort();
	auto scissorRect = swapChain_->GetRect();
	command_->GetCommandList()->RSSetViewports(1, &viewPort);
	command_->GetCommandList()->RSSetScissorRects(1, &scissorRect);
}

void SampleScene::EndRender()
{
	auto bbIdx = swapChain_->GetSwapChain()->GetCurrentBackBufferIndex();

	// Transit render target to present
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(swapChain_->GetCurrentBackBuffer()->GetResource().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	command_->GetCommandList()->ResourceBarrier(1, &barrier);

	command_->WaitCommand();
}

void SampleScene::Render()
{
	BeginRender();

	camera_.Update();
	void* rawPtr = cameraBuffer_->Map();
	if (rawPtr) {
		CameraMatrix* pCamera = static_cast<CameraMatrix*>(rawPtr);
		pCamera->view = camera_.GetView();
		pCamera->proj = camera_.GetProj();
		cameraBuffer_->Unmap();
	}

	command_->SetPipeline(lambert_);
	command_->SetGraphicsRootSig(sphere0RootSignature_);
	command_->SetDescriptorHeap(sphere0DescManager_);
	command_->SetGraphicsRootDescriptorTable(0, sphere0DescManager_);
	command_->SetGraphicsRoot32BitConstants(1, ColorConstants_);
	command_->AddDrawIndexed(sphere_, 1);

	// GUI
	{
		GUI_->BeginCommand();
		// Add GUI 
		ImGui::Text("Color");
		ImGui::SameLine();
		ImGui::ColorEdit4("##SurfaceColor", (float*)&diffuseColor_);
		GUI_->EndCommand();
		command_->DrawGUI(GUI_);
	}

	EndRender();

	swapChain_->GetSwapChain()->Present(1, 0);
}

SampleScene::SampleScene()
{
	
}

bool SampleScene::Init(const Application& app)
{
	device_.Init(L"NVIDIA");

	dxc_.Init();

	GUI_ = device_.CreateGUI(app.GetWindowHWND());

	command_ = device_.CreateCommand();

	SIZE size = { app.GetWindowWidth(), app.GetWindowHeight()};
	swapChain_ = device_.CreateSwapChain(command_, app.GetWindowHWND(), size);

	// Objects data
	string modelPath = string(MODEL_DIR) + "Suzanne.gltf";
	sphere_ = device_.CreateMesh(command_, modelPath);

	// Scene Items
	// Camera
	camera_.Init((float)app.GetWindowWidth() / (float)app.GetWindowHeight(), XMFLOAT3(0.0f, 0.0f, -5.0f));
	
	// Light
	light0_.pos = XMFLOAT4(10.0f, 10.0f, -5.0f, 1.0f);
	light0_.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	// Sphere0
	XMMATRIX modelMat = XMMatrixMultiply(XMMatrixRotationY(XMConvertToRadians(180)), XMMatrixIdentity());
	sphere0_.model = modelMat;
	sphere0_.invTransModel = XMMatrixTranspose(XMMatrixInverse(nullptr, sphere0_.model));
	
	// Resources
	cameraBuffer_ = device_.CreateBuffer(BufferType::Upload, Buffer::AlignForConstantBuffer(sizeof(CameraMatrix)), 1);
	void* rawPtr = cameraBuffer_->Map();
	if (rawPtr) {
		CameraMatrix* pCamera = static_cast<CameraMatrix*>(rawPtr);
		pCamera->view = camera_.GetView();
		pCamera->proj = camera_.GetProj();
		cameraBuffer_->Unmap();
	}

	light0Buffer_ = device_.CreateBuffer(BufferType::Upload, Buffer::AlignForConstantBuffer(sizeof(Light)), 1);
	rawPtr = light0Buffer_->Map();
	if (rawPtr) {
		Light* pLight = static_cast<Light*>(rawPtr);
		*pLight = light0_;
		light0Buffer_->Unmap();
	}

	sphere0Buffer_ = device_.CreateBuffer(BufferType::Upload, Buffer::AlignForConstantBuffer(sizeof(TransformMatrix)), 1);
	rawPtr = sphere0Buffer_->Map();
	if (rawPtr) {
		TransformMatrix* pTransformMatrix = static_cast<TransformMatrix*>(rawPtr);
		*pTransformMatrix = sphere0_;
		sphere0Buffer_->Unmap();
	}

	// Shaders
	wstring shaderPath = wstring(SHADER_DIR) + L"lambert.hlsl";
	simpleVS_ = dxc_.CreateShader(ShaderType::Vertex, shaderPath, L"VSmain");
	lambertPS_ = dxc_.CreateShader(ShaderType::Pixel, shaderPath, L"PSmain");

	// Descriptor Manager
	sphere0DescManager_ = device_.CreateDescriptorManager(
		HeapType::Resource, 
		{
			{ cameraBuffer_, ViewType::CBV, 0},
			{ light0Buffer_, ViewType::CBV, 1},
			{ sphere0Buffer_, ViewType::CBV, 2}
		}
	);
	// RootSignature
	sphere0RootSignature_ = device_.CreateRootSignature(
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
		{
			{RootParamType::DescTable,	sphere0DescManager_},
			{RootParamType::Constant,	DirectRootParamDesc{3, 4}},
		}
	);

	diffuseColor_ = { {1.0f, 0.0f, 0.0f, 1.0f} };
	ColorConstants_ = std::make_shared<Constants>(static_cast<void*>(&diffuseColor_), 4);

	// Graphics pipeline
	vector<D3D12_INPUT_ELEMENT_DESC> inputLayouts =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};
	GraphicsDesc lambertDesc(inputLayouts);
	lambertDesc.rootSignature_ = sphere0RootSignature_;
	lambertDesc.VS_ = simpleVS_;
	lambertDesc.PS_ = lambertPS_;
	lambert_ = device_.CreateGraphicsPipeline(
		lambertDesc
	);

	
	return true;
}