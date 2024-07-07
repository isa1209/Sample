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
	//デバッグレイヤーの有効化
	EnableDebugLayer();

	window.cbSize = sizeof(WNDCLASSEX);
	window.lpfnWndProc = (WNDPROC)WindowProcedure;		//コールバック関数の指定
	window.lpszClassName = _T("DX12Sample");		//アプリケーションクラス名
	window.hInstance = GetModuleHandle(nullptr);		//ハンドルの取得

	RegisterClassEx(&window);

	RECT wrc = { 0, 0, windowWidth, windowHeight };		//ウィンドウサイズ

	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);		//ウィンドウサイズの補正

	//ウィンドウオブジェクトの作成
	hwnd = CreateWindow(
		window.lpszClassName,		//クラス名
		_T("DX12テスト"),			//タイトルバーの文字
		WS_OVERLAPPEDWINDOW,		//ウィンドウの種類
		CW_USEDEFAULT,				//表示ｘ座標
		CW_USEDEFAULT,				//表示ｙ座標
		wrc.right - wrc.left,		//幅
		wrc.bottom - wrc.top,		//高さ
		nullptr,					//親ウィンドウハンドル
		nullptr,					//メニューハンドル
		window.hInstance,			//呼び出しアプリケーションハンドル
		nullptr						//追加パラメーター
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
	//メッセージループ
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

		ProcessInput();	//入力処理
		UpdateGame();	//ワールド更新
		GenerateOutput();	//出力処理
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

	//描画する方のバッファーのインデックスを取得
	auto bbIdx = swapChain->GetCurrentBackBufferIndex();

	//リソースバリアを指定
	D3D12_RESOURCE_BARRIER BarrierDesc = {};

	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = backBuffers[bbIdx];
	BarrierDesc.Transition.Subresource = 0;

	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	cmdList->ResourceBarrier(1, &BarrierDesc);

	//バッファーの位置を計算
	auto rtvH = rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//レンダーターゲットを描画する方のバッファーに指定
	cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

	//指定色でクリア
	cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

	//パイプラインステートをセット
	cmdList->SetPipelineState(pipelineState);

	//ルートシグネチャをセット
	cmdList->SetGraphicsRootSignature(rootsignature);

	//ディスクリプタヒープを指定
	cmdList->SetDescriptorHeaps(1, &texDescHeap);

	cmdList->SetGraphicsRootDescriptorTable(
		0, texDescHeap->GetGPUDescriptorHandleForHeapStart()
	);

	//ビューポート、シザー矩形をセット
	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissorrect);

	//プリミティブトポロジをセット
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//頂点バッファーをセット
	cmdList->IASetVertexBuffers(0, 1, &vbview);

	//インデックスバッファーをセット
	cmdList->IASetIndexBuffer(&ibView);

	//描画命令
	cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	cmdList->ResourceBarrier(1, &BarrierDesc);

	//クローズ
	cmdList->Close(); 
	
	//コマンドリストの実行
	ID3D12CommandList* cmdLists[]{ cmdList };
	cmdQueue->ExecuteCommandLists(1, cmdLists);

	//シグナル
	cmdQueue->Signal(fence, ++fenceVal);

	//GPU側のフェンス値とCPU側のフェンス値を比較
	if (fence -> GetCompletedValue() != fenceVal)
	{
		//イベントハンドルを取得
		auto event = CreateEvent(nullptr, false, false, nullptr);

		fence->SetEventOnCompletion(fenceVal, event);

		//イベントが発生するまで待機
		WaitForSingleObject(event, INFINITE);

		//イベントを閉じる
		CloseHandle(event);
	}

	//リセット
	result = cmdAllocator->Reset();
	if (result != S_OK)
	{
		printf("コマンドアロケーターのリセットに失敗");
		mIsRunning = false;
	}

	result = cmdList->Reset(cmdAllocator, nullptr);
	if (result != S_OK)
	{
		printf("コマンドリストのリセットに失敗");
		mIsRunning = false;
	}

	//画面のスワップ（第一引数で垂直同期を指定）
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
	//画像をロード
	if (IsFailed(LoadImage_D3D12()))
	{
		printf("画像のロードに失敗");
		return false;
	}

	//Device,Factoryを生成
	if (IsFailed(GenerateD3D12Device_Factory())) 
	{
		printf("デバイスオブジェクトの作成に失敗");
		return false;
	};

	//コマンドリスト・アロケーター・キューの初期化
	if (IsFailed(GenerateCommandList_Allocator_Queue()))
	{
		printf("コマンドリスト・アロケーター・キューの初期化に失敗");
		return false;
	}

	//スワップチェーンの生成
	if (IsFailed(GenerateSwapChain()))
	{
		printf("スワップチェーンの初期化に失敗");
		return false;
	}

	//ディスクリプタヒープの生成
	if (IsFailed(GenerateDescriptorHeap()))
	{
		printf("ディスクリプタヒープの初期化に失敗");
		return false;
	}

	//スワップチェーンとディスクリプタを接続
	if (IsFailed(RelateSwapChain_DescriptorHeap()))
	{
		printf("スワップチェーンとディスクリプタの接続に失敗");
		return false;
	}

	//フェンス生成
	if (IsFailed(GenerateFence()))
	{
		printf("フェンスの生成に失敗");
		return false;
	}

	//頂点バッファーの生成
	if (IsFailed(GenerateVertexBuffer()))
	{
		printf("頂点バッファーの生成に失敗");
		return false;
	}

	//インデックスバッファーの生成
	if (IsFailed(GenerateIndexBuffer()))
	{
		printf("インデックスバッファーの生成に失敗");
		return false;
	}

	//テクスチャバッファーの生成
	if (IsFailed(GenerateTextureBuffer()))
	{
		printf("テクスチャバッファーの作成に失敗");
		return false;
	}

	//シェーダー読み込み
	if (IsFailed(LoadShader()))
	{
		printf("シェーダー読み込みに失敗");

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


	//ルートシグネチャの生成
	if (IsFailed(GenerateRootSignature()))
	{
		printf("ルートシグネチャの生成に失敗");
		return false;
	}


	//グラフィックスパイプラインステートの生成
	if (IsFailed(GenerateGraphicsPipelineState()))
	{
		printf("グラフィックスパイプラインステートの作成に失敗");
		return false;
	}

	//ビューポート、シザー矩形を設定
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

	//合うフィーチャーレベルでDirect3Dデバイスを作成
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

	//DXGIFactoryオブジェクトの生成
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


	//コマンドキューの初期化
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

	//レンダーターゲットビュー用ディスクリプタヒープを作成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	results.push_back(
		device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeap))
		);

	//テクスチャ用ディスクリプタヒープを作成
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
		

		//メモリ位置を計算
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

	//頂点情報を頂点バッファーにコピー
	Vertex* vertMap = nullptr;

	results.push_back(
		vertBuff->Map(0, nullptr, (void**)&vertMap)
	);

	std::copy(std::begin(vertices), std::end(vertices), vertMap);

	vertBuff->Unmap(0, nullptr);

	//頂点バッファービューの設定
	vbview.BufferLocation = vertBuff->GetGPUVirtualAddress();	//GPU上のバッファーの仮想アドレス
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

	//インデックス情報をインデックスバッファーにコピー
	unsigned short* mappedIdx = nullptr;

	results.push_back(
		idxBuff->Map(0, nullptr, (void**)&mappedIdx)
	);

	std::copy(std::begin(indices), std::end(indices), mappedIdx);

	idxBuff->Unmap(0, nullptr);

	//インデックスバッファービューの設定
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

	//テクスチャバッファーにコピー
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
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,	//デバッグモード、最適化スキップ
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
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,	//デバッグモード、最適化スキップ
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

	//頂点レイアウトの作成
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {

		{	//座標情報
			"POSITION",									//セマンティクス名
			0,											//同じセマンティクス名の時のインデックス
			DXGI_FORMAT_R32G32B32_FLOAT,				//フォーマット
			0,											//入力スロットインデックス
			D3D12_APPEND_ALIGNED_ELEMENT,				//データのオフセット位置（連続）
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	//一頂点ごとにレイアウト
			0											//一度に描画するインスタンスの数
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
