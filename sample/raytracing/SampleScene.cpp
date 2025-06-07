#include "SampleScene.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

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
		pCamera->invViewProj = camera_.GetInvViewProj();
		pCamera->invView = camera_.GetInvView();
		pCamera->cameraPosition = camera_.GetPos();
		cameraBuffer_->Unmap();
	}

	//command_->SetComputeResourceSet(sphere0ResourceSet_);
	command_->SetComputeRootSig(*suzanneRootSignature_);
	command_->SetDescriptorHeap(*suzanneDescManager_);
	command_->SetComputeRootDescriptorTable(0, *suzanneDescManager_);
	command_->SetComputeRoot32BitConstants(1, 4, &diffuseColor_);
	command_->GetStableCommandList()->SetPipelineState1(raytracingStates_->GetStateObject().Get());
	D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = rayTracing_->GetDispatchRayDesc();
	command_->GetStableCommandList()->DispatchRays(&dispatchRaysDesc);

	command_->CopyBuffer(*outputTexture_, *(swapChain_->GetCurrentBackBuffer()));

	// GUI
	{
		GUI_->BeginCommand();
		// Add GUI 
		ImGui::Text("Color");
		ImGui::SameLine();
		ImGui::ColorEdit4("##SurfaceColor", (float*)&diffuseColor_);
		GUI_->EndCommand();
		command_->DrawGUI(*GUI_);
	}

	EndRender();

	swapChain_->GetSwapChain()->Present(1, 0);
}

SampleScene::SampleScene()
{
	
}

bool SampleScene::Init(const Application& app)
{
	if (!device_.Init(L"NVIDIA")) {
		cerr << "Failed to init Device" << endl;
		return false;
	}

	if (!dxc_.Init()) {
		return false;
	}

	GUI_ = device_.CreateGUI(app.GetWindowHWND());

	command_ = device_.CreateCommand();

	SIZE size = { app.GetWindowWidth(), app.GetWindowHeight()};
	swapChain_ = device_.CreateSwapChain(command_, app.GetWindowHWND(), size);

	// ASMesh
	string modelPath = string(MODEL_DIR) + "Suzanne.gltf";
	suzanneASMesh_ = device_.CreateASMesh(command_, modelPath);
	suzanneMesh_ = device_.CreateMesh(command_, modelPath);

	// Scene Items
	// Camera
	camera_.Init((float)app.GetWindowWidth() / (float)app.GetWindowHeight(), XMFLOAT3(0.0f, 0.0f, -5.0f));
	// Light
	light0_.pos = XMFLOAT4(10.0f, 10.0f, -5.0f, 1.0f);
	light0_.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	suzanneBLAS_ = device_.CreateBLAS(command_, *suzanneASMesh_);
	sceneTLAS_ = device_.CreateTLAS(
		command_,
		{
			{XMMatrixIdentity(), 0, suzanneBLAS_, D3D12_RAYTRACING_INSTANCE_FLAG_NONE}
		}
	);
	
	// Resources
	cameraBuffer_ = device_.CreateBuffer(BufferType::Upload, Buffer::AlignForConstantBuffer(sizeof(CameraMatrix)), 1);
	void* rawPtr = cameraBuffer_->Map();
	if (rawPtr) {
		CameraMatrix* pCamera = static_cast<CameraMatrix*>(rawPtr);
		pCamera->view = camera_.GetView();
		pCamera->proj = camera_.GetProj();
		pCamera->invViewProj = camera_.GetInvViewProj();
		pCamera->invView = camera_.GetInvView();
		pCamera->cameraPosition = XMFLOAT4(0.0f, 0.0f, -5.0f, 1.0f);
		cameraBuffer_->Unmap();
	}

	light0Buffer_ = device_.CreateBuffer(BufferType::Upload, Buffer::AlignForConstantBuffer(sizeof(Light)), 1);
	rawPtr = light0Buffer_->Map();
	if (rawPtr) {
		Light* pLight = static_cast<Light*>(rawPtr);
		*pLight = light0_;
		light0Buffer_->Unmap();
	}

	outputTexture_ = device_.CreateTexture(
		TextureDim::Tex2D,
		TextureType::Unordered,
		0, DXGI_FORMAT_R8G8B8A8_UNORM, app.GetWindowWidth(), app.GetWindowHeight(), 1
	);

	// Shaders
	wstring shaderPath = wstring(SHADER_DIR) + L"raytracing.hlsl";
	rayGen_		= dxc_.CreateShader(ShaderType::RayTracing, shaderPath, L"rayGeneration");
	closestHit_ = dxc_.CreateShader(ShaderType::RayTracing, shaderPath, L"closestHit");
	miss_		= dxc_.CreateShader(ShaderType::RayTracing, shaderPath, L"miss");

	// Descriptor Manager
	suzanneDescManager_ = device_.CreateDescriptorManager(
		HeapType::Resource, 
		{
			{ cameraBuffer_,					ViewType::CBV, 0},
			{ light0Buffer_,					ViewType::CBV, 1},
			{ sceneTLAS_->GetASBuffer(),		ViewType::SRV, 0},
			{ suzanneMesh_->GetVertexBuffer(),	ViewType::SRV, 1},
			{ suzanneMesh_->GetIndexBuffer(),	ViewType::SRV, 2},
			// NOTE : Need output UAV
			{ outputTexture_,					ViewType::UAV, 0}
		}
	);
	// RootSignature
	suzanneRootSignature_ = device_.CreateRootSignature(
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
		{
			{RootParamType::DescTable,	DescTableRootParamDesc{*suzanneDescManager_}},
			{RootParamType::Constant,	DirectRootParamDesc{2, 4}}
		}
	);

	suzanneResourceSet_ = std::make_shared<ResourceSet>(
		suzanneRootSignature_,
		// NOTE : Error occur when substitute BindResource
		std::initializer_list<std::variant<std::shared_ptr<DescriptorManager>, std::shared_ptr<Buffer>, Constants>>
		{
			std::shared_ptr<DescriptorManager>{ suzanneDescManager_ },
			Constants{static_cast<void*>(&diffuseColor_), 4, 0}
		}
	);

	// StateObject
	raytracingStates_ = device_.CreateStateObject(
		{
			StateObjectType::Raytracing,
			StateObjectDesc::RayTracingDesc
			{
				suzanneRootSignature_,
				{
					{rayGen_, ShaderStage::RayGen},
				},
				{
					{miss_, ShaderStage::Miss},
				},
				{
					{
						L"HitGroup",
						{ closestHit_, ShaderStage::ClosestHit },
					}
				},
				{3 * sizeof(float), 2 * sizeof(float), 1}
			}
		}
	);

	rayTracing_ = device_.CreateRaytracing(
		*raytracingStates_,
		app.GetWindowWidth(),
		app.GetWindowHeight(),
		1
	);

	diffuseColor_ = { {1.0f, 0.0f, 0.0f, 1.0f} };

	
	return true;
}