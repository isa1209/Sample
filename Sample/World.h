#pragma once

#include <Windows.h>
#include <tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <string>
#include <DirectXTEX.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "DirectXTEX.lib")

using namespace std;
using namespace DirectX;

//頂点情報
struct Vertex
{
	XMFLOAT3 pos;	//xyz
	XMFLOAT2 uv;	//uv
};

//テクスチャ情報（ダミー）
struct TexRGBA
{
	unsigned char R, G, B, A;
};

class  World
{
public:
	 World();
	~ World();

	bool Initialize();	//ゲームを初期化する

	void RunLoop();		//ゲームオーバーまでゲームループを実行

	void ShutDown();	//ゲームをシャットダウン

private:

	//ゲームループのためのヘルパー関数群
	void ProcessInput();
	void UpdateGame();
	void GenerateOutput();

	bool mIsRunning;

	void EnableDebugLayer();	//デバッグレイヤーの有効化

	bool DX12Initialize();

	static LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);	//ウィンドウプロシージャ

	WNDCLASSEX window = {};
	HWND hwnd = nullptr;
	ID3D12Device* device = nullptr;
	IDXGIFactory6* dxgiFactory = nullptr;
	IDXGISwapChain4* swapChain = nullptr;
	ID3D12GraphicsCommandList* cmdList = nullptr;
	ID3D12CommandAllocator* cmdAlloc = nullptr;
	ID3D12CommandQueue* cmdQueue = nullptr;
	ID3D12DescriptorHeap* rtvHeap = nullptr;
	ID3D12Fence* fence = nullptr;	
	std::vector<ID3D12Resource*> backBuffers;
	ID3DBlob* vsBlob = nullptr;
	ID3DBlob* psBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	ID3D12PipelineState* pipelineState = nullptr;
	ID3DBlob* rootSigBlob = nullptr;
	ID3D12RootSignature* rootsignature = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vbview = {};
	D3D12_VIEWPORT viewport = {};
	D3D12_RECT scissorrect = {};
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ID3D12DescriptorHeap* texDescHeap = nullptr;
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	const Image* img;


	UINT64 fenceVal = 0;	//CPU側のフェンス値

	D3D_FEATURE_LEVEL levels[4] = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	int windowWidth = 1270;
	int windowHeight = 980;

	float clearColor[4] = {1.0f, 1.0f, 0.0f, 1.0f};

	Vertex vertices[4] = {
		{{ -0.4f, -0.7f, 0.0f }, {0.0f, 1.0f}},
		{{ -0.4f, 0.7f, 0.0f }, {0.0f, 0.0f}},
		{{ 0.4f, -0.7f, 0.0f }, {1.0f, 1.0f}},
		{{ 0.4f, 0.7f, 0.0f }, {1.0f, 0.0f}}
	
	};

	std::vector<TexRGBA> texturedata;

	unsigned short indices[6] = {
		0,1,2,
		2,1,3
	};

	bool IsFailed  (std::vector<HRESULT>);

	std::vector<HRESULT> LoadImage_D3D12();

	std::vector<HRESULT> GenerateD3D12Device_Factory();

	std::vector<HRESULT> GenerateCommandList_Allocator_Queue();

	std::vector<HRESULT> GenerateSwapChain();

	std::vector<HRESULT> GenerateDescriptorHeap();

	std::vector<HRESULT> RelateSwapChain_DescriptorHeap();

	std::vector<HRESULT> GenerateFence();

	std::vector<HRESULT> GenerateVertexBuffer();

	std::vector<HRESULT> GenerateIndexBuffer();

	std::vector<HRESULT> GenerateTextureBuffer();

	std::vector<HRESULT> LoadShader();

	std::vector<HRESULT> GenerateRootSignature();

	std::vector<HRESULT> GenerateGraphicsPipelineState();

	void SettingViewport_ScissorRect();

 
};