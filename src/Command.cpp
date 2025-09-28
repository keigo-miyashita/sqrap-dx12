#include "Command.hpp"

#include "Descriptormanager.hpp"
#include "Device.hpp"
#include "Fence.hpp"
#include "Indirect.hpp"
#include "Gui.hpp"
#include "Mesh.hpp"
#include "Pipeline.hpp"
#include "Resource.hpp"
#include "Rootsignature.hpp"
#include "Swapchain.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	void Command::CreateCommandList()
	{
		commandType_ = commandType_;
		HRESULT result = pDevice_->GetDevice()->CreateCommandAllocator(commandType_, IID_PPV_ARGS(commandAllocator_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw std::runtime_error("Failed to CreateCommandAllocator : " + to_string(result));
		}
		wstring cmdAllocName = L"CommandAllocator";
		commandAllocator_->SetName((cmdAllocName + name_).c_str());
		result = pDevice_->GetDevice()->CreateCommandList(0, commandType_, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(commandList_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw std::runtime_error("Failed to CreateCommandList : " + to_string(result));
		}
		wstring cmdListName = L"CommandList";
		commandList_->SetName((cmdListName + name_).c_str());
	}

	void Command::InitializeStableCommandList()
	{
		HRESULT result = commandList_->QueryInterface(IID_PPV_ARGS(stableCommandList_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw std::runtime_error("Failed to QueryInterface for StableCommandList : " + to_string(result));
		}
		stableCommandList_->SetName((L"StableCommandList" + name_).c_str());
	}

	void Command::InitializeLatestCommandList()
	{
		HRESULT result = commandList_->QueryInterface(IID_PPV_ARGS(latestCommandList_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw std::runtime_error("Failed to QueryInterface for LatestCommandList : " + to_string(result));
		}
		latestCommandList_->SetName((L"LatestCommandList" + name_).c_str());
	}

	/*void Command::CreateCommandQueue()
	{
		D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
		cmdQueueDesc.Type = commandType_;
		cmdQueueDesc.NodeMask = 0;
		cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		HRESULT result = pDevice_->GetDevice()->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(commandQueue_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw std::runtime_error("Failed to CreateCommandQueue : " + to_string(result));
		}
		wstring cmdQueueName = L"CommandQueue";
		commandQueue_->SetName((cmdQueueName + name_).c_str());
	}*/

	Command::Command(const Device& device, D3D12_COMMAND_LIST_TYPE commandType, wstring name) : pDevice_(&device), commandType_(commandType), name_(name)
	{
		CreateCommandList();

		InitializeStableCommandList();

		InitializeLatestCommandList();

		//CreateCommandQueue();

		fence_ = pDevice_->CreateFence(name);
	}

	void Command::BeginRender(SwapChainHandle swapchain)
	{
		auto bbIdx = swapchain->GetSwapChain()->GetCurrentBackBufferIndex();

		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(swapchain->GetCurrentBackBuffer()->GetResource().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		commandList_->ResourceBarrier(1, &barrier);

		auto rtvHandle = swapchain->GetRtvHeap()->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += bbIdx * pDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		auto dsvHandle = swapchain->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart();
		commandList_->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

		// Clear depth buffer
		commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		// Clear display
		float clearColor[] = { 0.5f, 0.5f, 0.5f, 1.0f };
		commandList_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		// Set viewport and scissors
		auto viewPort = swapchain->GetViewPort();
		auto scissorRect = swapchain->GetRect();
		commandList_->RSSetViewports(1, &(viewPort));
		commandList_->RSSetScissorRects(1, &(scissorRect));
	}

	void Command::EndRender(SwapChainHandle swapchain)
	{
		auto bbIdx = swapchain->GetSwapChain()->GetCurrentBackBufferIndex();

		// Transit render target to present
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(swapchain->GetCurrentBackBuffer()->GetResource().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		commandList_->ResourceBarrier(1, &barrier);

		WaitCommand();

		swapchain->GetSwapChain()->Present(1, 0);
	}

	void Command::AddDrawIndexed(MeshHandle mesh, UINT numInstances)
	{
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList_->IASetVertexBuffers(0, 1, mesh->GetVBViewPtr());
		commandList_->IASetIndexBuffer(mesh->GetIBViewPtr());
		commandList_->DrawIndexedInstanced(mesh->GetNumIndices(), 1, 0, 0, 0);
	}

	void Command::AddDrawIndexedLine(MeshHandle mesh, UINT numInstances)
	{
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		commandList_->IASetVertexBuffers(0, 1, mesh->GetVBViewPtr());
		commandList_->IASetIndexBuffer(mesh->GetIBViewPtr());
		commandList_->DrawIndexedInstanced(mesh->GetNumIndices(), 1, 0, 0, 0);
	}

	void Command::Barrier(UINT numBarriers, D3D12_RESOURCE_BARRIER* pBarriers)
	{
		commandList_->ResourceBarrier(numBarriers, pBarriers);
	}

	void Command::CopyBuffer(ResourceHandle srcResource, ResourceHandle destResource)
	{
		vector<CD3DX12_RESOURCE_BARRIER> rscBarriers;
		if (srcResource->GetResourceState() != D3D12_RESOURCE_STATE_COPY_SOURCE) {
			auto srcBarrier = CD3DX12_RESOURCE_BARRIER::Transition(srcResource->GetResource().Get(), srcResource->GetResourceState(), D3D12_RESOURCE_STATE_COPY_SOURCE);
			rscBarriers.push_back(srcBarrier);
		}
		if (destResource->GetResourceState() != D3D12_RESOURCE_STATE_COPY_DEST) {
			auto destBarrier = CD3DX12_RESOURCE_BARRIER::Transition(destResource->GetResource().Get(), destResource->GetResourceState(), D3D12_RESOURCE_STATE_COPY_DEST);
			rscBarriers.push_back(destBarrier);
		}

		if (rscBarriers.size() != 0) {
			commandList_->ResourceBarrier(rscBarriers.size(), rscBarriers.data());
		}
		commandList_->CopyResource(destResource->GetResource().Get(), srcResource->GetResource().Get());

		rscBarriers.clear();
		if (srcResource->GetResourceState() != D3D12_RESOURCE_STATE_COPY_SOURCE) {
			auto srcbarrier = CD3DX12_RESOURCE_BARRIER::Transition(srcResource->GetResource().Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, srcResource->GetResourceState());
			rscBarriers.push_back(srcbarrier);
		}
		if (destResource->GetResourceState() != D3D12_RESOURCE_STATE_COPY_DEST) {
			auto destbarrier = CD3DX12_RESOURCE_BARRIER::Transition(destResource->GetResource().Get(), D3D12_RESOURCE_STATE_COPY_DEST, destResource->GetResourceState());
			rscBarriers.push_back(destbarrier);
		}
		if (rscBarriers.size() != 0) {
			commandList_->ResourceBarrier(rscBarriers.size(), rscBarriers.data());
		}
	}

	void Command::CopyBufferRegion(BufferHandle srcBuffer, UINT srcOffset, BufferHandle destBuffer, UINT destOffset, UINT numBytes)
	{
		vector<CD3DX12_RESOURCE_BARRIER> rscBarriers;
		if (srcBuffer->GetResourceState() != D3D12_RESOURCE_STATE_COPY_SOURCE) {
			auto srcBarrier = CD3DX12_RESOURCE_BARRIER::Transition(srcBuffer->GetResource().Get(), srcBuffer->GetResourceState(), D3D12_RESOURCE_STATE_COPY_SOURCE);
			rscBarriers.push_back(srcBarrier);
		}
		if (destBuffer->GetResourceState() != D3D12_RESOURCE_STATE_COPY_DEST) {
			auto destBarrier = CD3DX12_RESOURCE_BARRIER::Transition(destBuffer->GetResource().Get(), destBuffer->GetResourceState(), D3D12_RESOURCE_STATE_COPY_DEST);
			rscBarriers.push_back(destBarrier);
		}

		if (rscBarriers.size() != 0) {
			commandList_->ResourceBarrier(rscBarriers.size(), rscBarriers.data());
		}
		commandList_->CopyBufferRegion(destBuffer->GetResource().Get(), destOffset, srcBuffer->GetResource().Get(), srcOffset, numBytes);

		rscBarriers.clear();
		if (srcBuffer->GetResourceState() != D3D12_RESOURCE_STATE_COPY_SOURCE) {
			auto srcbarrier = CD3DX12_RESOURCE_BARRIER::Transition(srcBuffer->GetResource().Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, srcBuffer->GetResourceState());
			rscBarriers.push_back(srcbarrier);
		}
		if (destBuffer->GetResourceState() != D3D12_RESOURCE_STATE_COPY_DEST) {
			auto destbarrier = CD3DX12_RESOURCE_BARRIER::Transition(destBuffer->GetResource().Get(), D3D12_RESOURCE_STATE_COPY_DEST, destBuffer->GetResourceState());
			rscBarriers.push_back(destbarrier);
		}
		if (rscBarriers.size() != 0) {
			commandList_->ResourceBarrier(rscBarriers.size(), rscBarriers.data());
		}
	}

	void Command::Dispatch(UINT threadX, UINT threadY, UINT threadZ)
	{
		commandList_->Dispatch(threadX, threadY, threadZ);
	}

	void Command::DispatchRays(RayTracingHandle rayTracing)
	{
		D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = rayTracing->GetDispatchRayDesc();
		stableCommandList_->DispatchRays(&dispatchRaysDesc);
	}

	void Command::DrawIndirect(MeshHandle mesh, IndirectHandle indirect, BufferHandle buffer, UINT maxCommandNum)
	{
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList_->IASetVertexBuffers(0, 1, mesh->GetVBViewPtr());
		commandList_->IASetIndexBuffer(mesh->GetIBViewPtr());
		commandList_->ExecuteIndirect(indirect->GetCommandSignature().Get(), maxCommandNum, buffer->GetResource().Get(), 0, nullptr, 0);
	}

	void Command::DrawGUI(GUIHandle GUI)
	{
		commandList_->SetDescriptorHeaps(1, GUI->GetImguiDescHeap().GetAddressOf());
		GUI->Draw(*this);
	}

	void Command::ExecuteIndirect(IndirectHandle indirect)
	{
		if (indirect->GetCounterBuffer() != nullptr) {
			commandList_->ExecuteIndirect(
				indirect->GetCommandSignature().Get(),
				indirect->GetMaxCommandCount(),
				indirect->GetArgumentBuffer()->GetResource().Get(),
				0,
				indirect->GetCounterBuffer()->GetResource().Get(),
				indirect->GetCounterBufferOffset()
			);
		}
		else {
			commandList_->ExecuteIndirect(
				indirect->GetCommandSignature().Get(),
				indirect->GetMaxCommandCount(),
				indirect->GetArgumentBuffer()->GetResource().Get(),
				0,
				nullptr,
				0
			);
		}
	}

	void Command::SetMesh(MeshHandle mesh)
	{
		commandList_->IASetVertexBuffers(0, 1, mesh->GetVBViewPtr());
		commandList_->IASetIndexBuffer(mesh->GetIBViewPtr());
	}

	void Command::SetComputeResource(RootSignatureHandle computeRootSig)
	{
		SetComputeRootSig(computeRootSig);
		int rootParamIndex = 0;
		for (const auto& rootParam_ : computeRootSig->GetRootParametersVec()) {
			if (rootParam_.rootParamType_ == RootParamType::DescTable) {
				DescriptorManagerHandle descManager = std::get<DescriptorManagerHandle>(rootParam_.rootParamDesc_);
				SetDescriptorHeap(descManager);
				SetComputeRootDescriptorTable(rootParamIndex, descManager);
			}
			else if (rootParam_.rootParamType_ == RootParamType::CBV) {
				DirectRootParamDesc directDesc = std::get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
				// NOTE : to handle CBV
			}
			else if (rootParam_.rootParamType_ == RootParamType::SRV) {
				DirectRootParamDesc directDesc = std::get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
				// NOTE : to handle SRV
			}
			else if (rootParam_.rootParamType_ == RootParamType::UAV) {
				DirectRootParamDesc directDesc = std::get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
				// NOTE : to handle UAV
			}
			else if (rootParam_.rootParamType_ == RootParamType::Constant) {
				DirectRootParamDesc directDesc = std::get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
				//SetComputeRoot32BitConstants(rootParamIndex, /*pointer*/ nullptr);
				commandList_->SetComputeRoot32BitConstants(rootParamIndex, directDesc.numConstant_, directDesc.constantPtr_, 0);
			}
			rootParamIndex++;
		}
	}

	void Command::SetComputeRoot32BitConstants(UINT rootParamIndex, ConstantsHandle constant)
	{
		commandList_->SetComputeRoot32BitConstants(rootParamIndex, constant->GetNumConstants(), constant->GetConstants(), 0);
	}

	void Command::SetComputeRootDescriptorTable(UINT rootParamIndex, DescriptorManagerHandle descManager)
	{
		commandList_->SetComputeRootDescriptorTable(rootParamIndex, descManager->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
	}

	void Command::SetComputeRootSig(RootSignatureHandle computeRootSig)
	{
		commandList_->SetComputeRootSignature(computeRootSig->GetRootSignature().Get());
	}

	void Command::SetDescriptorHeap(DescriptorManagerHandle descManager)
	{
		commandList_->SetDescriptorHeaps(1, descManager->GetDescriptorHeap().GetAddressOf());
	}

	void Command::SetGraphicsResource(RootSignatureHandle graphicsRootSig)
	{
		SetGraphicsRootSig(graphicsRootSig);
		int rootParamIndex = 0;
		for (const auto& rootParam_ : graphicsRootSig->GetRootParametersVec()) {
			if (rootParam_.rootParamType_ == RootParamType::DescTable) {
				DescriptorManagerHandle descManager = std::get<DescriptorManagerHandle>(rootParam_.rootParamDesc_);
				SetDescriptorHeap(descManager);
				SetGraphicsRootDescriptorTable(rootParamIndex, descManager);
			}
			else if (rootParam_.rootParamType_ == RootParamType::CBV) {
				DirectRootParamDesc directDesc = std::get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
				// NOTE : to handle CBV
			}
			else if (rootParam_.rootParamType_ == RootParamType::SRV) {
				DirectRootParamDesc directDesc = std::get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
				// NOTE : to handle SRV
			}
			else if (rootParam_.rootParamType_ == RootParamType::UAV) {
				DirectRootParamDesc directDesc = std::get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
				// NOTE : to handle UAV
			}
			else if (rootParam_.rootParamType_ == RootParamType::Constant) {
				DirectRootParamDesc directDesc = std::get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
				//SetGraphicsRoot32BitConstants(rootParamIndex, /*pointer*/ nullptr);
				commandList_->SetGraphicsRoot32BitConstants(rootParamIndex, directDesc.numConstant_, directDesc.constantPtr_, 0);
			}
			rootParamIndex++;
		}
	}

	void Command::SetGraphicsRoot32BitConstants(UINT rootParamIndex, ConstantsHandle constant)
	{
		commandList_->SetGraphicsRoot32BitConstants(rootParamIndex, constant->GetNumConstants(), constant->GetConstants(), 0);
	}

	void Command::SetGraphicsRootDescriptorTable(UINT rootParamIndex, DescriptorManagerHandle descManager)
	{
		commandList_->SetGraphicsRootDescriptorTable(rootParamIndex, descManager->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
	}

	void Command::SetGraphicsRootSig(RootSignatureHandle graphicsRootSig)
	{
		commandList_->SetGraphicsRootSignature(graphicsRootSig->GetRootSignature().Get());
	}

	void Command::SetRayTracingResource(RootSignatureHandle rayTracingRootSig)
	{
		SetComputeResource(rayTracingRootSig);
	}

	void Command::SetPipeline(GraphicsPipelineHandle graphicsPipeline)
	{
		commandList_->SetPipelineState(graphicsPipeline->GetPipelineState().Get());
	}

	void Command::SetPipeline(ComputePipelineHandle computePipeline)
	{
		commandList_->SetPipelineState(computePipeline->GetPipelineState().Get());
	}

	void Command::SetRayTracingState(StateObjectHandle stateObject)
	{
		stableCommandList_->SetPipelineState1(stateObject->GetStateObject().Get());
	}

	bool Command::WaitCommand(QueueType queueType)
	{
		if (!fence_->WaitCommand(*this, queueType)) {
			return false;
		}
		return true;
	}

	void Command::InitDataToBuffer(BufferHandle buffer, void* pData, UINT strideSize, UINT numElement)
	{
		BufferHandle uploadBuffer_ = pDevice_->CreateBuffer(BufferType::Upload, strideSize, numElement);

		uploadBuffer_->Upload(pData, strideSize, numElement);

		this->CopyBufferRegion(uploadBuffer_, 0, buffer, 0, strideSize * numElement);

		this->WaitCommand();
	}

	D3D12_COMMAND_LIST_TYPE Command::GetCommandType()
	{
		return commandType_;
	}

	ComPtr<ID3D12CommandAllocator> Command::GetCommandAllocator() const
	{
		return commandAllocator_;
	}

	ComPtr<ID3D12GraphicsCommandList> Command::GetCommandList() const
	{
		return commandList_;
	}

	ComPtr<StableCommandList> Command::GetStableCommandList() const
	{
		return stableCommandList_;
	}

	ComPtr<LatestCommandList> Command::GetLatestCommandList() const
	{
		return latestCommandList_;
	}

	/*ComPtr<ID3D12CommandQueue> Command::GetCommandQueue() const
	{
		return commandQueue_;
	}*/

	Fence& Command::GetFence()
	{
		return *fence_;
	}
}