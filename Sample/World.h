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

//���_���
struct Vertex
{
	XMFLOAT3 pos;	//xyz
	XMFLOAT2 uv;	//uv
};

//�e�N�X�`�����i�_�~�[�j
struct TexRGBA
{
	unsigned char R, G, B, A;
};

class  World
{
public:
	 World();
	~ World();

	bool Initialize();	//�Q�[��������������

	void RunLoop();		//�Q�[���I�[�o�[�܂ŃQ�[�����[�v�����s

	void ShutDown();	//�Q�[�����V���b�g�_�E��

private:

	//�Q�[�����[�v�̂��߂̃w���p�[�֐��Q
	void ProcessInput();
	void UpdateGame();
	void GenerateOutput();

	bool mIsRunning;

	void EnableDebugLayer();	//�f�o�b�O���C���[�̗L����

	bool DX12Initialize();

	static LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);	//�E�B���h�E�v���V�[�W��

	WNDCLASSEX window = {};
	HWND hwnd = nullptr;							//�E�B���h�E�n���h��
	ID3D12Device* device = nullptr;					//�f�o�C�X�I�u�W�F�N�g
	IDXGIFactory6* dxgiFactory = nullptr;			
	IDXGISwapChain4* swapChain = nullptr;			//�X���b�v�`�F�[���I�u�W�F�N�g

	ID3D12GraphicsCommandList* cmdList = nullptr;	//�R�}���h���X�g
	ID3D12CommandAllocator* cmdAllocator = nullptr;	//�R�}���h�A���P�[�^�[
	ID3D12CommandQueue* cmdQueue = nullptr;			///�R�}���h�L���[

	ID3D12DescriptorHeap* rtvHeap = nullptr;		//�����_�[�^�[�Q�b�g�r���[Heap
	ID3D12DescriptorHeap* texDescHeap = nullptr;

	ID3D12Fence* fence = nullptr;					//�t�F���X
	UINT64 fenceVal = 0;							//CPU���̃t�F���X�l
	
	std::vector<ID3D12Resource*> backBuffers;		//�o�b�N�o�b�t�@�[�I�u�W�F�N�g

	ID3DBlob* vsBlob = nullptr;						//���_�V�F�[�_�[�I�u�W�F�N�g
	ID3DBlob* psBlob = nullptr;						//�s�N�Z���V�F�[�_�[�I�u�W�F�N�g
	ID3DBlob* errorBlob = nullptr;

	ID3D12PipelineState* pipelineState = nullptr;	//�p�C�v���C���X�e�[�g

	ID3DBlob* rootSigBlob = nullptr;
	ID3D12RootSignature* rootsignature = nullptr;	//���[�g�V�O�l�`��

	D3D12_VERTEX_BUFFER_VIEW vbview = {};			//���_�o�b�t�@�r���[
	D3D12_INDEX_BUFFER_VIEW ibView = {};			//�C���f�b�N�X�o�b�t�@�[�r���[

	D3D12_VIEWPORT viewport = {};					//�r���[�|�[�g
	D3D12_RECT scissorrect = {};					//�V�U�[��`
	
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	const Image* img;

	int windowWidth = 1270;
	int windowHeight = 980;

	D3D_FEATURE_LEVEL levels[4] = {					//�t���[�`���[���x��
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	float clearColor[4] = {1.0f, 1.0f, 0.0f, 1.0f};

	Vertex vertices[4] = {							//���_�ʒu
		{{ -0.4f, -0.7f, 0.0f }, {0.0f, 1.0f}},
		{{ -0.4f, 0.7f, 0.0f }, {0.0f, 0.0f}},
		{{ 0.4f, -0.7f, 0.0f }, {1.0f, 1.0f}},
		{{ 0.4f, 0.7f, 0.0f }, {1.0f, 0.0f}}
	
	};

	unsigned short indices[6] = {					//���_�C���f�b�N�X
		0,1,2,
		2,1,3
	};

	std::vector<TexRGBA> texturedata;				//�_�~�[�e�N�X�`��

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