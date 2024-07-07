#include "World.h"


World::World()
{
	backBuffers.resize(2);

	texturedata.resize(256 * 256);

	for (auto &rgba : texturedata)
	{
		rgba.R = rand() % 256;
		rgba.G = rand() % 256;
		rgba.B = rand() % 256;
		rgba.A = 256;
	}

	mIsRunning = true;
}

World::~World()
{
}

bool World::Initialize()
{
	//�f�o�b�O���C���[�̗L����
	EnableDebugLayer();

	window.cbSize = sizeof(WNDCLASSEX);
	window.lpfnWndProc = (WNDPROC)WindowProcedure;		//�R�[���o�b�N�֐��̎w��
	window.lpszClassName = _T("DX12Sample");		//�A�v���P�[�V�����N���X��
	window.hInstance = GetModuleHandle(nullptr);		//�n���h���̎擾

	RegisterClassEx(&window);

	RECT wrc = { 0, 0, windowWidth, windowHeight };		//�E�B���h�E�T�C�Y

	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);		//�E�B���h�E�T�C�Y�̕␳

	//�E�B���h�E�I�u�W�F�N�g�̍쐬
	hwnd = CreateWindow(
		window.lpszClassName,		//�N���X��
		_T("DX12�e�X�g"),			//�^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,		//�E�B���h�E�̎��
		CW_USEDEFAULT,				//�\�������W
		CW_USEDEFAULT,				//�\�������W
		wrc.right - wrc.left,		//��
		wrc.bottom - wrc.top,		//����
		nullptr,					//�e�E�B���h�E�n���h��
		nullptr,					//���j���[�n���h��
		window.hInstance,			//�Ăяo���A�v���P�[�V�����n���h��
		nullptr						//�ǉ��p�����[�^�[
	);

	ShowWindow(hwnd, SW_SHOW);

	if (DX12Initialize() == false)
	{
		return false;
	}




	return true;
}

void World::RunLoop()
{
	//���b�Z�[�W���[�v
	MSG msg = {};

	while (mIsRunning)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT)
		{
			break;
		}

		ProcessInput();	//���͏���
		UpdateGame();	//���[���h�X�V
		GenerateOutput();	//�o�͏���
	}
}

void World::ShutDown()
{
	UnregisterClass(window.lpszClassName, window.hInstance);
}

void World::ProcessInput()
{
}

void World::UpdateGame()
{

}

void World::GenerateOutput()
{
	HRESULT result;

	//�`�悷����̃o�b�t�@�[�̃C���f�b�N�X���擾
	auto bbIdx = swapChain->GetCurrentBackBufferIndex();

	//���\�[�X�o���A���w��
	D3D12_RESOURCE_BARRIER BarrierDesc = {};

	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = backBuffers[bbIdx];
	BarrierDesc.Transition.Subresource = 0;

	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	cmdList->ResourceBarrier(1, &BarrierDesc);

	//�o�b�t�@�[�̈ʒu���v�Z
	auto rtvH = rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//�����_�[�^�[�Q�b�g��`�悷����̃o�b�t�@�[�Ɏw��
	cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

	//�w��F�ŃN���A
	cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

	//�p�C�v���C���X�e�[�g���Z�b�g
	cmdList->SetPipelineState(pipelineState);

	//���[�g�V�O�l�`�����Z�b�g
	cmdList->SetGraphicsRootSignature(rootsignature);

	//�f�B�X�N���v�^�q�[�v���w��
	cmdList->SetDescriptorHeaps(1, &texDescHeap);

	cmdList->SetGraphicsRootDescriptorTable(
		0, texDescHeap->GetGPUDescriptorHandleForHeapStart()
	);

	//�r���[�|�[�g�A�V�U�[��`���Z�b�g
	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissorrect);

	//�v���~�e�B�u�g�|���W���Z�b�g
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//���_�o�b�t�@�[���Z�b�g
	cmdList->IASetVertexBuffers(0, 1, &vbview);

	//�C���f�b�N�X�o�b�t�@�[���Z�b�g
	cmdList->IASetIndexBuffer(&ibView);

	//�`�施��
	cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	cmdList->ResourceBarrier(1, &BarrierDesc);

	//�N���[�Y
	cmdList->Close(); 
	
	//�R�}���h���X�g�̎��s
	ID3D12CommandList* cmdLists[]{ cmdList };
	cmdQueue->ExecuteCommandLists(1, cmdLists);

	//�V�O�i��
	cmdQueue->Signal(fence, ++fenceVal);

	//GPU���̃t�F���X�l��CPU���̃t�F���X�l���r
	if (fence -> GetCompletedValue() != fenceVal)
	{
		//�C�x���g�n���h�����擾
		auto event = CreateEvent(nullptr, false, false, nullptr);

		fence->SetEventOnCompletion(fenceVal, event);

		//�C�x���g����������܂őҋ@
		WaitForSingleObject(event, INFINITE);

		//�C�x���g�����
		CloseHandle(event);
	}

	//���Z�b�g
	result = cmdAllocator->Reset();
	if (result != S_OK)
	{
		printf("�R�}���h�A���P�[�^�[�̃��Z�b�g�Ɏ��s");
		mIsRunning = false;
	}

	result = cmdList->Reset(cmdAllocator, nullptr);
	if (result != S_OK)
	{
		printf("�R�}���h���X�g�̃��Z�b�g�Ɏ��s");
		mIsRunning = false;
	}

	//��ʂ̃X���b�v�i�������Ő����������w��j
	swapChain->Present(1, 0);



	
}

void World::EnableDebugLayer()
{
	ID3D12Debug* debuglayer = nullptr;

	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debuglayer));

	debuglayer->EnableDebugLayer();

	debuglayer->Release();
}

bool World::DX12Initialize()
{
	//�摜�����[�h
	if (IsFailed(LoadImage_D3D12()))
	{
		printf("�摜�̃��[�h�Ɏ��s");
		return false;
	}

	//Device,Factory�𐶐�
	if (IsFailed(GenerateD3D12Device_Factory())) 
	{
		printf("�f�o�C�X�I�u�W�F�N�g�̍쐬�Ɏ��s");
		return false;
	};

	//�R�}���h���X�g�E�A���P�[�^�[�E�L���[�̏�����
	if (IsFailed(GenerateCommandList_Allocator_Queue()))
	{
		printf("�R�}���h���X�g�E�A���P�[�^�[�E�L���[�̏������Ɏ��s");
		return false;
	}

	//�X���b�v�`�F�[���̐���
	if (IsFailed(GenerateSwapChain()))
	{
		printf("�X���b�v�`�F�[���̏������Ɏ��s");
		return false;
	}

	//�f�B�X�N���v�^�q�[�v�̐���
	if (IsFailed(GenerateDescriptorHeap()))
	{
		printf("�f�B�X�N���v�^�q�[�v�̏������Ɏ��s");
		return false;
	}

	//�X���b�v�`�F�[���ƃf�B�X�N���v�^��ڑ�
	if (IsFailed(RelateSwapChain_DescriptorHeap()))
	{
		printf("�X���b�v�`�F�[���ƃf�B�X�N���v�^�̐ڑ��Ɏ��s");
		return false;
	}

	//�t�F���X����
	if (IsFailed(GenerateFence()))
	{
		printf("�t�F���X�̐����Ɏ��s");
		return false;
	}

	//���_�o�b�t�@�[�̐���
	if (IsFailed(GenerateVertexBuffer()))
	{
		printf("���_�o�b�t�@�[�̐����Ɏ��s");
		return false;
	}

	//�C���f�b�N�X�o�b�t�@�[�̐���
	if (IsFailed(GenerateIndexBuffer()))
	{
		printf("�C���f�b�N�X�o�b�t�@�[�̐����Ɏ��s");
		return false;
	}

	//�e�N�X�`���o�b�t�@�[�̐���
	if (IsFailed(GenerateTextureBuffer()))
	{
		printf("�e�N�X�`���o�b�t�@�[�̍쐬�Ɏ��s");
		return false;
	}

	//�V�F�[�_�[�ǂݍ���
	if (IsFailed(LoadShader()))
	{
		printf("�V�F�[�_�[�ǂݍ��݂Ɏ��s");

		std::string errstr;
		errstr.resize(errorBlob->GetBufferSize());
		std::copy_n(
			(char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			errstr.begin()
		);

		OutputDebugStringA(errstr.c_str());

		return false;
	}


	//���[�g�V�O�l�`���̐���
	if (IsFailed(GenerateRootSignature()))
	{
		printf("���[�g�V�O�l�`���̐����Ɏ��s");
		return false;
	}


	//�O���t�B�b�N�X�p�C�v���C���X�e�[�g�̐���
	if (IsFailed(GenerateGraphicsPipelineState()))
	{
		printf("�O���t�B�b�N�X�p�C�v���C���X�e�[�g�̍쐬�Ɏ��s");
		return false;
	}

	//�r���[�|�[�g�A�V�U�[��`��ݒ�
	SettingViewport_ScissorRect();


	

	return true;
}

LRESULT World::WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

bool World::IsFailed(std::vector<HRESULT> results)
{
	for (auto result : results)
	{
		if (FAILED(result)) return true;
	}

	return false;
}

std::vector<HRESULT> World::GenerateD3D12Device_Factory()
{
	std::vector<HRESULT> results;

	//�����t�B�[�`���[���x����Direct3D�f�o�C�X���쐬
	D3D_FEATURE_LEVEL featureLevel;
	HRESULT result;

	for (auto lv : levels)
	{
		result = D3D12CreateDevice(nullptr, lv, IID_PPV_ARGS(&device));


		if (result == S_OK)
		{
			featureLevel = lv;
			break;
		}
	}
	results.push_back(result);

	//DXGIFactory�I�u�W�F�N�g�̐���
	//result = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	results.push_back(
		CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory))
	);

	return results;
}

std::vector<HRESULT> World::GenerateCommandList_Allocator_Queue()
{
	std::vector<HRESULT> results;

	results.push_back(
		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator))
	);

	results.push_back(
		device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator, nullptr, IID_PPV_ARGS(&cmdList))
	);


	//�R�}���h�L���[�̏�����
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};

	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	results.push_back(
		device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&cmdQueue))
		);

	return results;
}

std::vector<HRESULT> World::GenerateSwapChain()
{
	std::vector<HRESULT> results;

	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};

	swapchainDesc.Width = windowWidth;
	swapchainDesc.Height = windowHeight;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	results.push_back(
		dxgiFactory->CreateSwapChainForHwnd(cmdQueue, hwnd, &swapchainDesc, nullptr, nullptr, (IDXGISwapChain1**)&swapChain)
	);
	

	return results;
}

std::vector<HRESULT> World::GenerateDescriptorHeap()
{
	std::vector<HRESULT> results;

	//�����_�[�^�[�Q�b�g�r���[�p�f�B�X�N���v�^�q�[�v���쐬
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	results.push_back(
		device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeap))
		);

	//�e�N�X�`���p�f�B�X�N���v�^�q�[�v���쐬
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};

	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	results.push_back(
		device->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&texDescHeap))
	);

	return results;
}

std::vector<HRESULT> World::RelateSwapChain_DescriptorHeap()
{
	std::vector<HRESULT> results;

	for (int i = 0; i < 2; ++i)
	{
		results.push_back(
			swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]))
		);
		

		//�������ʒu���v�Z
		D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += i * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		device->CreateRenderTargetView(backBuffers[i], nullptr, handle);
	}

	return results;
}

std::vector<HRESULT> World::GenerateFence()
{
	std::vector<HRESULT> results;

	results.push_back(
		device->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))
	);

	return results;
}

std::vector<HRESULT> World::GenerateVertexBuffer()
{
	std::vector<HRESULT> results;

	D3D12_HEAP_PROPERTIES heapprop = {};

	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resdesc = {};

	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = sizeof(vertices);
	resdesc.Height = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels = 1;
	resdesc.Format = DXGI_FORMAT_UNKNOWN;
	resdesc.SampleDesc.Count = 1;
	resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource* vertBuff = nullptr;

	results.push_back(
		device->CreateCommittedResource(
			&heapprop,
			D3D12_HEAP_FLAG_NONE,
			&resdesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&vertBuff))
		);

	//���_���𒸓_�o�b�t�@�[�ɃR�s�[
	Vertex* vertMap = nullptr;

	results.push_back(
		vertBuff->Map(0, nullptr, (void**)&vertMap)
	);

	std::copy(std::begin(vertices), std::end(vertices), vertMap);

	vertBuff->Unmap(0, nullptr);

	//���_�o�b�t�@�[�r���[�̐ݒ�
	vbview.BufferLocation = vertBuff->GetGPUVirtualAddress();	//GPU��̃o�b�t�@�[�̉��z�A�h���X
	vbview.SizeInBytes = sizeof(vertices);
	vbview.StrideInBytes = sizeof(vertices[0]);

	return results;
}

std::vector<HRESULT> World::GenerateIndexBuffer()
{
	std::vector<HRESULT> results;

	D3D12_HEAP_PROPERTIES heapprop = {};

	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resdesc = {};

	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = sizeof(indices);
	resdesc.Height = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels = 1;
	resdesc.Format = DXGI_FORMAT_UNKNOWN;
	resdesc.SampleDesc.Count = 1;
	resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource* idxBuff = nullptr;

	results.push_back(
		device->CreateCommittedResource(
			&heapprop,
			D3D12_HEAP_FLAG_NONE,
			&resdesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&idxBuff))
	);

	//�C���f�b�N�X�����C���f�b�N�X�o�b�t�@�[�ɃR�s�[
	unsigned short* mappedIdx = nullptr;

	results.push_back(
		idxBuff->Map(0, nullptr, (void**)&mappedIdx)
	);

	std::copy(std::begin(indices), std::end(indices), mappedIdx);

	idxBuff->Unmap(0, nullptr);

	//�C���f�b�N�X�o�b�t�@�[�r���[�̐ݒ�
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeof(indices);

	return results;
}

std::vector<HRESULT> World::GenerateTextureBuffer()
{
	std::vector<HRESULT> results;

	D3D12_HEAP_PROPERTIES heapprop = {};

	heapprop.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	heapprop.CreationNodeMask = 0;
	heapprop.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC resDesc = {};

	resDesc.Format = metadata.format;
	resDesc.Width = metadata.width;
	resDesc.Height = metadata.height;
	resDesc.DepthOrArraySize = metadata.arraySize;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.MipLevels = metadata.mipLevels;
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Resource* texbuff = nullptr;

	results.push_back(
		device->CreateCommittedResource(
			&heapprop,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			nullptr,
			IID_PPV_ARGS(&texbuff))
	);

	//�e�N�X�`���o�b�t�@�[�ɃR�s�[
	results.push_back(
		texbuff->WriteToSubresource(
			0,
			nullptr,
			img->pixels,
			img -> rowPitch,
			img -> slicePitch)
	);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	device->CreateShaderResourceView(
		texbuff,
		&srvDesc,
		texDescHeap->GetCPUDescriptorHandleForHeapStart()
	);

	return results;
}

std::vector<HRESULT> World::LoadShader()
{
	std::vector<HRESULT> results;

	results.push_back(
		D3DCompileFromFile(
			L"BasicVertexShader.hlsl",
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"BasicVS",
			"vs_5_0",
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,	//�f�o�b�O���[�h�A�œK���X�L�b�v
			0,
			&vsBlob,
			&errorBlob)
	);

	results.push_back(
		D3DCompileFromFile(
			L"BasicPixelShader.hlsl",
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"BasicPS",
			"ps_5_0",
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,	//�f�o�b�O���[�h�A�œK���X�L�b�v
			0,
			&psBlob,
			&errorBlob)
	);

	return results;
}

std::vector<HRESULT> World::GenerateRootSignature()
{
	std::vector<HRESULT> results;

	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};

	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

	D3D12_DESCRIPTOR_RANGE descTblRange = {};

	descTblRange.NumDescriptors = 1;
	descTblRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descTblRange.BaseShaderRegister = 0;
	descTblRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootparam = {};

	rootparam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootparam.DescriptorTable.NumDescriptorRanges = 1;
	rootparam.DescriptorTable.pDescriptorRanges = &descTblRange;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};

	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.NumParameters = 1;
	rootSignatureDesc.pParameters = &rootparam;
	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 1;


	results.push_back(
		D3D12SerializeRootSignature(
			&rootSignatureDesc,
			D3D_ROOT_SIGNATURE_VERSION_1_0,
			&rootSigBlob,
			&errorBlob)
	);

	results.push_back(
		device->CreateRootSignature(
			0,
			rootSigBlob->GetBufferPointer(),
			rootSigBlob->GetBufferSize(),
			IID_PPV_ARGS(&rootsignature))
	);

	rootSigBlob->Release();

	return results;
}

std::vector<HRESULT> World::GenerateGraphicsPipelineState()
{
	std::vector<HRESULT> results;

	//���_���C�A�E�g�̍쐬
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {

		{	//���W���
			"POSITION",									//�Z�}���e�B�N�X��
			0,											//�����Z�}���e�B�N�X���̎��̃C���f�b�N�X
			DXGI_FORMAT_R32G32B32_FLOAT,				//�t�H�[�}�b�g
			0,											//���̓X���b�g�C���f�b�N�X
			D3D12_APPEND_ALIGNED_ELEMENT,				//�f�[�^�̃I�t�Z�b�g�ʒu�i�A���j
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	//�꒸�_���ƂɃ��C�A�E�g
			0											//��x�ɕ`�悷��C���X�^���X�̐�
		},

		{	//uv
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		}
	};

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};

	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpStateDesc = {};

	gpStateDesc.pRootSignature = rootsignature;

	gpStateDesc.VS.BytecodeLength = vsBlob->GetBufferSize();
	gpStateDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	gpStateDesc.PS.BytecodeLength = psBlob->GetBufferSize();
	gpStateDesc.PS.pShaderBytecode = psBlob->GetBufferPointer();

	gpStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	gpStateDesc.RasterizerState.MultisampleEnable = false;
	gpStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	gpStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpStateDesc.RasterizerState.DepthClipEnable = true;

	gpStateDesc.RasterizerState.FrontCounterClockwise = false;
	gpStateDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	gpStateDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	gpStateDesc.RasterizerState.AntialiasedLineEnable = false;
	gpStateDesc.RasterizerState.ForcedSampleCount = 0;
	gpStateDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	gpStateDesc.DepthStencilState.DepthEnable = false;
	gpStateDesc.DepthStencilState.StencilEnable = false;

	gpStateDesc.BlendState.AlphaToCoverageEnable = false;
	gpStateDesc.BlendState.IndependentBlendEnable = false;

	gpStateDesc.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	gpStateDesc.InputLayout.pInputElementDescs = inputLayout;
	gpStateDesc.InputLayout.NumElements = _countof(inputLayout);

	gpStateDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	gpStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	gpStateDesc.NumRenderTargets = 1;
	gpStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	gpStateDesc.SampleDesc.Count = 1;
	gpStateDesc.SampleDesc.Quality = 0;

	results.push_back(
		device->CreateGraphicsPipelineState(
			&gpStateDesc,
			IID_PPV_ARGS(&pipelineState))
	);

	return results;
}

void World::SettingViewport_ScissorRect()
{
	viewport.Width = (FLOAT)windowWidth;
	viewport.Height = (FLOAT)windowHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MaxDepth = 1.0f;
	viewport.MinDepth = 0.0f;

	scissorrect.top = 0;
	scissorrect.left = 0;
	scissorrect.right = scissorrect.left + windowWidth;
	scissorrect.bottom = scissorrect.right + windowHeight;
}

std::vector<HRESULT> World::LoadImage_D3D12()
{
	std::vector<HRESULT> result;

	result.push_back(
		LoadFromWICFile(
			L"img/TestIMG.JPG",
			WIC_FLAGS_NONE,
			&metadata,
			scratchImg
		)
	);

	img = scratchImg.GetImage(0, 0, 0);

	return result;
}
