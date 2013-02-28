// This is the main DLL file.
#include "stdafx.h"
#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>
#include "resource.h"
#include "Wrapper.h"
#include <math.h>
using namespace System;
#pragma warning ( disable : 4005 )
//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct SimpleVertex
{
    XMFLOAT3 Pos;
    XMFLOAT2 Tex;
};

struct CBNeverChanges
{
    XMMATRIX mView;
};

struct CBChangeOnResize
{
    XMMATRIX mProjection;
};

struct CBChangesEveryFrame
{
    XMMATRIX mWorld;
};


//--------------------------------------------------------------------------------------
// Global                           Variables
//--------------------------------------------------------------------------------------
HINSTANCE                           g_hInst                                          = NULL;
HWND                                g_hWnd                                           = NULL;
D3D_DRIVER_TYPE                     g_driverType                                     = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL                   g_featureLevel                                   = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*                       g_pd3dDevice                                     = NULL;
ID3D11DeviceContext*                g_pImmediateContext                              = NULL;
IDXGISwapChain*                     g_pSwapChain                                     = NULL;
ID3D11RenderTargetView*             g_pRenderTargetView                              = NULL;
ID3D11Texture2D*                    g_pDepthStencil                                  = NULL;
ID3D11DepthStencilView*             g_pDepthStencilView                              = NULL;
ID3D11VertexShader*                 g_pVertexShader                                  = NULL;
ID3D11VertexShader*                 g_pVertexShaderBullet                            = NULL;
ID3D11PixelShader*                  g_pPixelShader                                   = NULL;
ID3D11PixelShader*                  g_pPixelShaderBullet                             = NULL;
ID3D11InputLayout*                  g_pVertexLayout                                  = NULL;
ID3D11Buffer*                       g_pVertexBuffer                                  = NULL;
ID3D11Buffer*                       g_pIndexBuffer                                   = NULL;
ID3D11Buffer*                       g_pCBNeverChanges                                = NULL;
ID3D11Buffer*                       g_pCBChangeOnResize                              = NULL;
ID3D11Buffer*                       g_pCBChangesEveryFrame                           = NULL;
XMMATRIX                            g_World;
XMMATRIX                            g_World_bullet;
XMMATRIX                            g_View;
XMMATRIX                            g_Projection;
XMFLOAT4							g_vDirection( 0.0f, 1.0f, 0.0f, 0.0f );
XMFLOAT2							g_center (0.0f, 0.0f);

bool isShooting = false;

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitDevice();
void CleanupDevice();
void Render();
//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HWND hwnd)
{
	g_hWnd = hwnd;
	return S_OK;
}


//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DX11
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;

    hr = D3DX11CompileFromFile( szFileName, NULL, NULL, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
    if( FAILED(hr) )
    {

        if( pErrorBlob != NULL )
		{
			
            OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
		}
        if( pErrorBlob ) pErrorBlob->Release();
        return hr;
    }
    if( pErrorBlob ) pErrorBlob->Release();

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice(int panelWidth, int panelHeight)
{
    HRESULT hr = S_OK;

    RECT rc;

	rc.left   = rc.top = 0;
	rc.right  = panelWidth;
	rc.bottom = panelHeight;

    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

	g_center.x = (float)width /2;
	g_center.y = (float)height /2;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount                        = 1;
    sd.BufferDesc.Width                   = width;
    sd.BufferDesc.Height                  = height;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = g_hWnd;
    sd.SampleDesc.Count                   = 1;
    sd.SampleDesc.Quality                 = 0;
    sd.Windowed                           = TRUE;

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain( NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
                                            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );
        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
	{
		MessageBox(NULL, L"create device failed", L"Error", MB_OK);
		return hr;
	}

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = NULL;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
    if( FAILED( hr ) )
	{
		MessageBox(NULL, L"create RTV failed", L"Error", MB_OK);
		return hr;
	}

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &g_pRenderTargetView );
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory( &descDepth, sizeof(descDepth) );
    descDepth.Width              = width;
    descDepth.Height             = height;
    descDepth.MipLevels          = 1;
    descDepth.ArraySize          = 1;
    descDepth.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count   = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage              = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags          = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags     = 0;
    descDepth.MiscFlags          = 0;
    hr                           = g_pd3dDevice->CreateTexture2D( &descDepth, NULL, &g_pDepthStencil );
    if( FAILED( hr ) )
        return hr;

    // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory( &descDSV, sizeof(descDSV) );
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = g_pd3dDevice->CreateDepthStencilView( g_pDepthStencil, &descDSV, &g_pDepthStencilView );
    if( FAILED( hr ) )
        return hr;

    g_pImmediateContext->OMSetRenderTargets( 1, &g_pRenderTargetView, g_pDepthStencilView );

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width    = (FLOAT)width;
    vp.Height   = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );


	//OnResize(width, height);

    // Compile the vertex shader
    ID3DBlob* pVSBlob = NULL;
    hr = CompileShaderFromFile( L"DefaultEffect.fx", "VS", "vs_4_0", &pVSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( NULL,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

    // Create the vertex shader
    hr = g_pd3dDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader );
    if( FAILED( hr ) )
    {    
        pVSBlob->Release();
        return hr;
    }


	// Compile the vertex shader
	ID3DBlob* pVSBlobBullet = NULL;
	hr = CompileShaderFromFile( L"DefaultEffect.fx", "VS", "vs_4_0", &pVSBlobBullet );
	if( FAILED( hr ) )
	{
		MessageBox( NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
		return hr;
	}

	// Create the vertex shader
	hr = g_pd3dDevice->CreateVertexShader( pVSBlobBullet->GetBufferPointer(), pVSBlobBullet->GetBufferSize(), NULL, &g_pVertexShaderBullet );
	if( FAILED( hr ) )
	{    
		pVSBlobBullet->Release();
		return hr;
	}



    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE( layout );

    // Create the input layout
    hr = g_pd3dDevice->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
                                          pVSBlob->GetBufferSize(), &g_pVertexLayout );
    pVSBlob->Release();
    if( FAILED( hr ) )
        return hr;

    // Set the input layout
    g_pImmediateContext->IASetInputLayout( g_pVertexLayout );

    // Compile the pixel shader
    ID3DBlob* pPSBlob = NULL;
    hr = CompileShaderFromFile( L"DefaultEffect.fx", "PS", "ps_4_0", &pPSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( NULL,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }
    
    hr = g_pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader );
    pPSBlob->Release();
    if( FAILED( hr ) )
        return hr;

	ID3DBlob* pPSBlobBullet = NULL;
	hr = CompileShaderFromFile( L"DefaultEffect.fx", "PS_Bullet", "ps_4_0", &pPSBlobBullet );
	if( FAILED( hr ) )
	{
		MessageBox( NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
		return hr;
	}
	hr = g_pd3dDevice->CreatePixelShader( pPSBlobBullet->GetBufferPointer(), pPSBlobBullet->GetBufferSize(), NULL, &g_pPixelShaderBullet );
	pPSBlobBullet->Release();
	if( FAILED( hr ) )
		return hr;


    // Create vertex buffer

	int vertexCount = 7;

	float scale = 0.2f;

    SimpleVertex vertices[] =
    {
        { XMFLOAT3( -1.0f, -1.0f, 0.0f ), XMFLOAT2( -1.0f, -1.0f ) },
        { XMFLOAT3( -1.0f, 1.0f, 0.0f ), XMFLOAT2( -1.0f, 1.0f ) },
        { XMFLOAT3( 1.0f, -1.0f, 0.0f ), XMFLOAT2( 1.0f, -1.0f ) },
        { XMFLOAT3( 1.0f, 1.0f, 0.0f ), XMFLOAT2( 1.0f, 1.0f ) },

		{ XMFLOAT3( 0.0f * scale, 2.0f * scale, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f* scale, -1.0f * scale, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3( -1.0f* scale, -1.0f * scale, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		
		
    };

    D3D11_BUFFER_DESC bd;
    ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( SimpleVertex ) * vertexCount;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory( &InitData, sizeof(InitData) );
    InitData.pSysMem = vertices;
    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pVertexBuffer );
    if( FAILED( hr ) )
        return hr;

    // Set vertex buffer
    UINT stride = sizeof( SimpleVertex );
    UINT offset = 0;
    g_pImmediateContext->IASetVertexBuffers( 0, 1, &g_pVertexBuffer, &stride, &offset );

    // Create index buffer    
    WORD indices[] =
    {
        0,1,2,
        2,1,3,

		0,1,2,
    };

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( WORD ) * 9;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    InitData.pSysMem = indices;
    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pIndexBuffer );
    if( FAILED( hr ) )
        return hr;

    // Set index buffer
    g_pImmediateContext->IASetIndexBuffer( g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );

    // Set primitive topology
    g_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    // Create the constant buffers
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(CBNeverChanges);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = g_pd3dDevice->CreateBuffer( &bd, NULL, &g_pCBNeverChanges );
    if( FAILED( hr ) )
        return hr;
    
    bd.ByteWidth = sizeof(CBChangeOnResize);
    hr = g_pd3dDevice->CreateBuffer( &bd, NULL, &g_pCBChangeOnResize );
    if( FAILED( hr ) )
        return hr;
    
    bd.ByteWidth = sizeof(CBChangesEveryFrame);
    hr = g_pd3dDevice->CreateBuffer( &bd, NULL, &g_pCBChangesEveryFrame );
    if( FAILED( hr ) )
        return hr;

    // Initialize the world matrices
    g_World = XMMatrixIdentity();
	g_World_bullet = XMMatrixIdentity();

    // Initialize the view matrix
    XMVECTOR Eye = XMVectorSet( 0.0f, 0.0f, -6.0f, 0.0f );
    XMVECTOR At = XMVectorSet( 0.0f, 0.0f, 0.0f, 0.0f );
    XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
    g_View = XMMatrixLookAtLH( Eye, At, Up );

    CBNeverChanges cbNeverChanges;
    cbNeverChanges.mView = XMMatrixTranspose( g_View );
    g_pImmediateContext->UpdateSubresource( g_pCBNeverChanges, 0, NULL, &cbNeverChanges, 0, 0 );

	g_Projection = XMMatrixPerspectiveFovLH( XM_PIDIV4, width / (FLOAT)height, 0.01f, 100.0f );

	CBChangeOnResize cbChangesOnResize;
	cbChangesOnResize.mProjection = XMMatrixTranspose( g_Projection );
	g_pImmediateContext->UpdateSubresource( g_pCBChangeOnResize, 0, NULL, &cbChangesOnResize, 0, 0 );
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
    if( g_pImmediateContext ) g_pImmediateContext->ClearState();
    if( g_pCBNeverChanges ) g_pCBNeverChanges->Release();
    if( g_pCBChangeOnResize ) g_pCBChangeOnResize->Release();
    if( g_pCBChangesEveryFrame ) g_pCBChangesEveryFrame->Release();
    if( g_pVertexBuffer ) g_pVertexBuffer->Release();
    if( g_pIndexBuffer ) g_pIndexBuffer->Release();
    if( g_pVertexLayout ) g_pVertexLayout->Release();
    if( g_pVertexShader ) g_pVertexShader->Release();
	if( g_pVertexShaderBullet ) g_pVertexShaderBullet->Release();
    if( g_pPixelShader ) g_pPixelShader->Release();
	if( g_pPixelShaderBullet ) g_pPixelShaderBullet->Release();
    if( g_pDepthStencil ) g_pDepthStencil->Release();
    if( g_pDepthStencilView ) g_pDepthStencilView->Release();
    if( g_pRenderTargetView ) g_pRenderTargetView->Release();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pImmediateContext ) g_pImmediateContext->Release();
    if( g_pd3dDevice ) g_pd3dDevice->Release();
}




//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
void RenderFrame()
{
	static float lastTime = (float)timeGetTime(); 

	float currTime = (float)timeGetTime();
	float t = (currTime - lastTime)/1000.f;

    //
    // Clear the back buffer
    //
    float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    g_pImmediateContext->ClearRenderTargetView( g_pRenderTargetView, ClearColor );

    //
    // Clear the depth buffer to 1.0 (max depth)
    //
    g_pImmediateContext->ClearDepthStencilView( g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );

    //
    // Update variables that change once per frame
    //
    CBChangesEveryFrame cb;
    cb.mWorld = XMMatrixTranspose( g_World );
    g_pImmediateContext->UpdateSubresource( g_pCBChangesEveryFrame, 0, NULL, &cb, 0, 0 );

    
    g_pImmediateContext->VSSetShader( g_pVertexShader, NULL, 0 );
    g_pImmediateContext->VSSetConstantBuffers( 0, 1, &g_pCBNeverChanges );
    g_pImmediateContext->VSSetConstantBuffers( 1, 1, &g_pCBChangeOnResize );
    g_pImmediateContext->VSSetConstantBuffers( 2, 1, &g_pCBChangesEveryFrame );
    g_pImmediateContext->PSSetShader( g_pPixelShader, NULL, 0 );
    g_pImmediateContext->DrawIndexed( 6, 0, 0 );

	if (isShooting)
	{

		XMFLOAT2 defaultDirection(0.f, 1.f);
		XMVECTOR newDir =  XMVectorSet( g_vDirection.x - g_center.x , g_center.y - g_vDirection.y , 0.f, 0.f);
		FXMVECTOR defaultDir = XMVectorSet(defaultDirection.x, defaultDirection.y, 0.f, 0.f);

		auto dot = XMVectorGetX( XMVector2Dot( XMVector2Normalize(defaultDir), XMVector2Normalize(newDir)));
		float angle =  XMScalarACos(dot);

		if (g_vDirection.x > g_center.x)
		{
			angle *= -1.0f;
		}

		auto rotation = XMMatrixRotationZ(angle);
		auto moveDir = XMVector2Normalize( XMVector3Transform(defaultDir, rotation));
		auto translation = XMMatrixTranslationFromVector(XMVectorScale(moveDir, t));

		g_World_bullet._11 = rotation._11;
		g_World_bullet._12 = rotation._12;
		g_World_bullet._21 = rotation._21;
		g_World_bullet._22 = rotation._22;
		g_World_bullet *= translation;

		cb.mWorld = XMMatrixTranspose( g_World_bullet );
		g_pImmediateContext->UpdateSubresource( g_pCBChangesEveryFrame, 0, NULL, &cb, 0, 0 );
		g_pImmediateContext->VSSetShader( g_pVertexShaderBullet, NULL, 0 );
		g_pImmediateContext->PSSetShader( g_pPixelShaderBullet, NULL, 0 );
		g_pImmediateContext->PSSetConstantBuffers( 2, 1, &g_pCBChangesEveryFrame );
		g_pImmediateContext->DrawIndexed( 3, 6, 4 );
	}
	
    g_pSwapChain->Present( 0, 0 );

	lastTime = currTime;
}

void setMatrix()
{
	g_World_bullet = XMMatrixIdentity();
}

Wrapper::TEEngine::TEEngine(System::IntPtr window, int width, int height)
{	
	if( FAILED( InitWindow( (HWND)window.ToPointer()) ) )
		return;

	InitDevice(width, height);	
	return ;
}

void Wrapper::TEEngine::Shoot(int x, int y)
{
	g_vDirection.x = x;
	g_vDirection.y = y;
	setMatrix();
	isShooting = true;
}

void Wrapper::TEEngine::Render()
{
	RenderFrame();
}

Wrapper::TEEngine::~TEEngine()
{
	CleanupDevice();
}
