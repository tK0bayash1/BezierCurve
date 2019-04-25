#include <Windows.h>
#include <DirectXMath.h>
#include <d3d11.h>
#include <d3dcompiler.h>

using namespace DirectX;

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

// 一つの頂点情報を格納する構造体
struct VERTEX {
	XMFLOAT3 V;
};

// GPU(シェーダ側)へ送る数値をまとめた構造体
struct CONSTANT_BUFFER {
	XMMATRIX mWVP;
};

#define WIN_STYLE WS_OVERLAPPEDWINDOW
int CWIDTH;     // クライアント領域の幅
int CHEIGHT;    // クライアント領域の高さ

HWND WHandle;
const char *ClassName = "Temp_Window";

IDXGISwapChain *pSwapChain;
ID3D11Device *pDevice;
ID3D11DeviceContext *pDeviceContext;
ID3D11RenderTargetView *pBackBuffer_RTV;
ID3D11InputLayout *pVertexLayout;
ID3D11VertexShader *pVertexShader;
ID3D11PixelShader *pPixelShader;
ID3D11GeometryShader *pGeometryShader;
ID3D11Buffer *pConstantBuffer;
ID3D11Buffer *pVertexBuffer;
ID3D11RasterizerState *pRasterizerState;

static float x = 0;

LRESULT CALLBACK WinProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hInstancePrev, LPSTR pCmdLine, int nCmdShow) {

	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = WinProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = ClassName;
	RegisterClass(&wc);

	WHandle = CreateWindow(ClassName, "Bezier Curve", WIN_STYLE, CW_USEDEFAULT, CW_USEDEFAULT, 1000, 800, NULL, NULL, hInstance, NULL);
	if (WHandle == NULL) return 0;
	ShowWindow(WHandle, nCmdShow);

	MSG msg = { 0 };
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			float clearColor[4] = { 0.1, 0.1, 0.1, 1 };
			pDeviceContext->ClearRenderTargetView(pBackBuffer_RTV, clearColor);

			XMVECTOR eye_pos = XMVectorSet(0.0f, 0.0f, -2.0f, 1.0f);        // 視点位置
			XMVECTOR eye_lookat = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);      // 視点方向
			XMVECTOR eye_up = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);          // 視点上方向
			XMMATRIX View = XMMatrixLookAtLH(eye_pos, eye_lookat, eye_up);  // 左手座標系のビュー行列
			XMMATRIX Proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, (FLOAT)CWIDTH / (FLOAT)CHEIGHT, 0.1f, 110.0f);  // 左手座標系のパースペクティブ射影行列
			XMMATRIX World = XMMatrixRotationY(x);                          // z軸を回転軸とした回転行列

			// パラメータの受け渡し
			D3D11_MAPPED_SUBRESOURCE pdata;
			CONSTANT_BUFFER cb;
			cb.mWVP = XMMatrixTranspose(World * View * Proj);                               // リソースへ送る値をセット
			pDeviceContext->Map(pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &pdata); // GPUからのリソースアクセスを一時止める
			memcpy_s(pdata.pData, pdata.RowPitch, (void*)(&cb), sizeof(cb));                // リソースへ値を送る
			pDeviceContext->Unmap(pConstantBuffer, 0);                                       // GPUからのリソースアクセスを再開

			// 描画実行
			pDeviceContext->Draw(3, 0);
			pSwapChain->Present(0, 0);
		}
	}

	return 0;
}


LRESULT CALLBACK WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {
	case WM_CREATE:
	{

		// ----- パイプラインの準備 -----
		RECT csize;
		GetClientRect(hwnd, &csize);
		CWIDTH = csize.right;
		CHEIGHT = csize.bottom;

		DXGI_SWAP_CHAIN_DESC scd = { 0 };
		scd.BufferCount = 1;
		scd.BufferDesc.Width = CWIDTH;
		scd.BufferDesc.Height = CHEIGHT;
		scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scd.BufferDesc.RefreshRate.Numerator = 60;
		scd.BufferDesc.RefreshRate.Denominator = 1;
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scd.OutputWindow = hwnd;
		scd.SampleDesc.Count = 1;
		scd.SampleDesc.Quality = 0;
		scd.Windowed = TRUE;
		D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_11_0;
		D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, &fl, 1, D3D11_SDK_VERSION, &scd, &pSwapChain, &pDevice, NULL, &pDeviceContext);

		ID3D11Texture2D *pbbTex;
		pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pbbTex);
		pDevice->CreateRenderTargetView(pbbTex, NULL, &pBackBuffer_RTV);
		pbbTex->Release();

		// ビューポートの設定
		D3D11_VIEWPORT vp;
		vp.Width = CWIDTH;
		vp.Height = CHEIGHT;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;

		// シェーダの設定
		ID3DBlob *pCompileVS = NULL;
		ID3DBlob *pCompilePS = NULL;
		ID3DBlob *pCompileGS = NULL;
		D3DCompileFromFile(L"shader.hlsl", NULL, NULL, "VS", "vs_5_0", NULL, 0, &pCompileVS, NULL);
		pDevice->CreateVertexShader(pCompileVS->GetBufferPointer(), pCompileVS->GetBufferSize(), NULL, &pVertexShader);
		D3DCompileFromFile(L"shader.hlsl", NULL, NULL, "PS", "ps_5_0", NULL, 0, &pCompilePS, NULL);
		pDevice->CreatePixelShader(pCompilePS->GetBufferPointer(), pCompilePS->GetBufferSize(), NULL, &pPixelShader);
		D3DCompileFromFile(L"shader.hlsl", NULL, NULL, "GS", "gs_5_0", NULL, 0, &pCompileGS, NULL);
		pDevice->CreateGeometryShader(pCompileGS->GetBufferPointer(), pCompileGS->GetBufferSize(), NULL, &pGeometryShader);

		// 頂点レイアウト
		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		pDevice->CreateInputLayout(layout, 1, pCompileVS->GetBufferPointer(), pCompileVS->GetBufferSize(), &pVertexLayout);
		pCompileVS->Release();
		pCompilePS->Release();

		// 定数バッファの設定
		D3D11_BUFFER_DESC cb;
		cb.ByteWidth = sizeof(CONSTANT_BUFFER);
		cb.Usage = D3D11_USAGE_DYNAMIC;
		cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cb.MiscFlags = 0;
		cb.StructureByteStride = 0;
		pDevice->CreateBuffer(&cb, NULL, &pConstantBuffer);

		// ポリゴンの頂点データの作成とそのバッファの設定
		VERTEX vertices[] = {
			XMFLOAT3(-0.5f, 0.5f, 0.0f),
			XMFLOAT3(0.8f, 0.3f, 0.0f),
			XMFLOAT3(0.5f, -0.5f, 0.0f),
		};
		D3D11_BUFFER_DESC bd;
		bd.ByteWidth = sizeof(VERTEX) * 3;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = vertices;
		pDevice->CreateBuffer(&bd, &InitData, &pVertexBuffer);

		// ラスタライザの設定
		D3D11_RASTERIZER_DESC rdc = {};
		rdc.FillMode = D3D11_FILL_SOLID;    // D3D11_FILL_WIREFRAMEでワイヤーフレーム, D3D11_FILL_SOLID
		rdc.CullMode = D3D11_CULL_NONE;
		rdc.FrontCounterClockwise = TRUE;
		pDevice->CreateRasterizerState(&rdc, &pRasterizerState);

		// ----- パイプラインの構築 -----
		UINT stride = sizeof(VERTEX);
		UINT offset = 0;
		pDeviceContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);      // 頂点データをセット
		pDeviceContext->IASetInputLayout(pVertexLayout);                             // 頂点レイアウトをセット
		pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);  // データの入力種類を指定
		pDeviceContext->OMSetRenderTargets(1, &pBackBuffer_RTV, NULL);                   // レンダーターゲットビューのセット
		pDeviceContext->RSSetViewports(1, &vp);                                          // ビューポートのセット
		pDeviceContext->VSSetShader(pVertexShader, NULL, 0);                         // 頂点シェーダをセット
		pDeviceContext->PSSetShader(pPixelShader, NULL, 0);                              // ピクセルシェーダをセット
		pDeviceContext->GSSetShader(pGeometryShader, NULL, 0);
		//pDeviceContext->VSSetConstantBuffers(0, 1, &pConstantBuffer);                    // 定数バッファをセット
		pDeviceContext->GSSetConstantBuffers(0, 1, &pConstantBuffer);                    // 定数バッファをセット
		pDeviceContext->RSSetState(pRasterizerState);                                    // ラスタライザの設定

		return 0;
	}
	case WM_DESTROY:

		pSwapChain->Release();
		pDeviceContext->Release();
		pDevice->Release();

		pBackBuffer_RTV->Release();

		pRasterizerState->Release();
		pVertexShader->Release();
		pVertexLayout->Release();
		pPixelShader->Release();
		pConstantBuffer->Release();
		pVertexBuffer->Release();

		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}