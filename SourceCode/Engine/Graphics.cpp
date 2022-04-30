#include "Graphics.h"
#include "DirectX11.h"
#include "Sprite.h"
#include "Model.h"
#include "Input.h"
#include "Camera.h"
#include "BlendMode.h"
#include "MODEL.h"
#include "LIGHTING.h"
#include "IMGUI.h"
#include "SHADERS.h"
#include "../GUI.h"

using namespace DirectX;


//std::shared_ptr<MODEL>Stage;

HRESULT Graphics::Initialize(int Width, int Height, HWND hwnd)
{
#pragma region SETTING INITIALIZATION


    DirectX11* DX = DirectX11::Instance();
    if (FAILED(DirectX11::Instance()->Initialize(Width, Height, true, hwnd, false, 10000.0f, 0.01f)))
        assert(!"Failed to Initilize DirectX11 Class");

    D3D11_BUFFER_DESC cbd{};
    cbd.ByteWidth = sizeof(SCENE_CONSTANT_DATA);
    cbd.Usage = D3D11_USAGE_DEFAULT;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    HRESULT hr = DX->Device()->CreateBuffer(&cbd, nullptr, dxSceneConstantBuffer.ReleaseAndGetAddressOf());
    if (FAILED(hr))
        assert(!"Failed to create constant buffer");

#pragma endregion

    // Initializes the shader manager by inserting used shaders
    ShaderManager::Instance()->Initialize();

    // Creates alpha blend mode
    BlendModeManager::Instance()->Create(BlendModeManager::BLEND_MODE::ALPHA, DirectX11::Instance()->Device());

    // Initializes the camera
    Camera::Instance()->Initialize({ 0, 0, 1 }, { 0, 0, 0 });
    Camera::Instance()->SetRange(10);

    // Create a default directional light and initialize the manager
    LightingManager::Instance()->Create("Default", LIGHTING::L_TYPE::DIRECTIONAL);
    LightingManager::Instance()->Retrieve("Default")->SetDirection({ 0, -1, -0.1 });
    LightingManager::Instance()->Retrieve("Default")->SetColour({ 1.0f, 1.0f, 1.0f, 1.0f });
    GUI::Instance()->Initialize();

    return S_OK;
}



bool Graphics::Frame()
{
    IMGUI::Instance()->Execute();

    GUI::Instance()->Execute();


#pragma region CALLS RENDER (DO NOT PUT FUNCTIONS BELOW HERE)
    if (Render())
        return false;
#pragma endregion
}

bool Graphics::Render()
{

#pragma region BASE SETTINGS
    ID3D11DeviceContext* dc{ DirectX11::Instance()->DeviceContext() };
    DirectX11::Instance()->Begin({ 0.2f, 0.2f, 0.2f, 1.0f });

#pragma endregion
#pragma region CAMERA SETTINGS
    Camera::Instance()->Execute();
#pragma endregion
#pragma region SCENE_CONSTANTS

    // Scene Constant buffer update (Camera Settings and Light Directions)
    SCENE_CONSTANT_DATA data;
    XMMATRIX v{ Camera::Instance()->ViewMatrix() }, pr{ DirectX11::Instance()->ProjectionMatrix() };
    DirectX::XMStoreFloat4x4(&data.view_proj, v * pr);


    // Camera Position
    data.camera_position.x = Camera::Instance()->Position().x;
    data.camera_position.y = Camera::Instance()->Position().y;
    data.camera_position.z = Camera::Instance()->Position().z;
    data.camera_position.w = 1;
    data.ambientLightColour = { 0.2f, 0.2f, 0.2f, 1.0f};
    // Light Direction
    LightingManager::Instance()->Retrieve("Default")->WriteBuffer<DLIGHT_DATA>(&data.directional);


    dc->UpdateSubresource(dxSceneConstantBuffer.Get(), 0, 0, &data, 0, 0);
    dc->VSSetConstantBuffers(0, 1, dxSceneConstantBuffer.GetAddressOf());
    dc->PSSetConstantBuffers(0, 1, dxSceneConstantBuffer.GetAddressOf());
#pragma endregion

#pragma region PUT RENDER FUNCTIONS HERE 




    LightingManager::Instance()->RenderDebug();
    GUI::Instance()->Render();
    IMGUI::Instance()->Render();
    //Camera::Instance()->RenderDebug();


#pragma endregion

    INPUTMANAGER::Instance()->Execute();
    DirectX11::Instance()->End();
    return true;
}

void Graphics::Finalize()
{
    GUI::Instance()->Finalize();
}