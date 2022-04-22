#include "GUI.h"
#include <string>
#include "MODEL.h"
#include "Engine/ResourceManager.h"
#include "Engine/DXMath.h"
#include "Engine/COLLISION.h"
#include "Engine/Camera.h"
#include "Engine/Input.h"
#include "Engine/DROPMANAGER.h"
#include "Engine/RASTERIZER.h"
using namespace Math;
std::shared_ptr<MODEL>plane;
Vector3 scale{ 1, 1, 1 }, rotation, translation;
Vector4 quaternion;
std::shared_ptr<MODEL>model;
std::shared_ptr<MODEL>selected;
bool colliding{};
int m_selected_item = -1;
int s_type{};
int selected_anim{};
std::string types[4] = { "DIFFUSE", "NORMAL", "BUMP", "EMPTY" };
std::string material_path;
std::string anim_name = "";
bool playAnim{};
bool wire{};
bool created_popup{};
bool error_format{}, error_path{};
bool texture_inserted{};
ImVec2 iMin, iMax;
bool empty{true};

/*------------------------------------------------GUI Class-----------------------------------------------------------------*/
/*--------------------------------------------------------------------------------------------------------------------------*/
/*-------------------------------------------------GUI Initialize()---------------------------------------------------------*/
/*--------------------------------------------------------------------------------------------------------------------------*/

void GUI::Initialize()
{
    model = std::make_shared<MODEL>();
    selected = std::make_shared<MODEL>();
    plane = std::make_shared<MODEL>();
    plane->Initialize("./Data/Model/Plane.mrs");
    plane->SetTransformation({ 100, 100, 100 }, {0, 0, 0}, { 0, -4, 0 });
    plane->SetTake(0);
    plane->Resource()->RemoveShader(L"PhongShader.fx");
    plane->Resource()->InsertShader(L"Base3DShader.fx");
    model->SetTransformation({ 1, 1, 1 }, {0, 0, 0}, {0, 0, 0});
}

/*-------------------------------------------------GUI Execute()---------------------------------------------------------*/

void GUI::Execute()
{

    int a = 0;
    ImGui::Begin("Model");
    std::string directory, format, path;
    IMGUI::Instance()->InputText("Model Path", &model_path);
    if (DROPMANAGER::Instance()->Loaded() && empty)
    {
        model_path = DROPMANAGER::Instance()->Path();
        if (hr == S_OK) {
            model = std::make_shared<MODEL>();
        }
        hr = model->Initialize(model_path);
        model->SetTake(0);
        model->SetFrame(0);
        Reset();
        empty = false;
    }

    ImGui::FileBrowser* browser{ IMGUI::Instance()->FileBrowser() };
    bool isEmpty{};
    static bool fileOpen{};

    if (ImGui::Button("Create Model") )
    {
        if (model_path == "")
        {
            browser->Open();
            browser->SetTitle("Open Model File");
            browser->SetTypeFilters({ ".fbx", ".mrs", ".obj", ".*" });
            isEmpty = fileOpen = true;
        }
        if (!isEmpty)
        {

            if (hr == S_OK) {
                model = std::make_shared<MODEL>();
            }
            hr = model->Initialize(model_path);
            model->SetTake(0);
            model->SetFrame(0);
            Reset();
        }
    }

    if (fileOpen)
    {
        browser->Display();
        if (browser->HasSelected())
        {
            model_path = browser->GetSelected().string();
            fileOpen = false;
            browser->Close();
        }
    }

    ImGui::FileBrowser* creator{IMGUI::Instance()->FileCreator()};
    static bool fileCreate{};
    if (ImGui::Button("Reset Camera"))
    {
        Camera::Instance()->ResetToTarget({ 0, 0, 0 });
    }
    if (hr == S_OK)
    {
        if (ImGui::Button("Export model"))
        {
            creator->Open();
            creator->SetTitle("Export model to");
            creator->SetTypeFilters({ ".mrs", ".*" });
            fileCreate = true;
        }

        if (fileCreate)
        {
            creator->Display();
            if (creator->HasSelected())
            {
                created_path = creator->GetSelected().string();
                format = { ".mrs" };
                std::filesystem::path path(created_path);
                path.replace_extension(format);
                model->resource->Recreate(path.string());
                fileCreate = false;
            }
        }
        //ImGui::InputText(" New File Name", (char*)created_path.c_str(), 256);
        //if (ImGui::Button("Recreate"))
        //{
        //    if (created_path == "") {
        //        fileCreate = empty = true;
        //        creator->Open();
        //        creator->SetTitle("Save as");
        //        creator->SetTypeFilters({ ".mrs" });
        //        /*std::string p{};
        //        directory = "./Generated/";
        //        format = ".mrs";
        //        p = created_path.c_str();
        //        p = directory + p + format;
        //        std::filesystem::path fp(p);

        //        if (std::filesystem::exists(fp))
        //            std::filesystem::remove(fp);
        //        model->Resource()->Recreate(fp.string());
        //        created_popup = true;*/
        //    }
        //    if(!empty)
        //    {
        //        format = { ".mrs" };
        //        std::filesystem::path path(created_path);
        //        path.replace_extension(format);
        //        model->resource->Recreate(path.string());
        //    }
        //}

        //IMGUI::Instance()->DisplayBrowser(&created_path, &fileCreate);

        IMGUI::Instance()->CreatePopup("Model Recreated! Check ./Generated for file", &created_popup);
    }
    //if (ImGui::Button("Reset Camera"))
    //    Camera::Instance()->ResetCamera();

    ImGui::End();

    if (hr == S_OK)
    {
        TransformUI();
        AnimationUI();
        MaterialUI();
        TimelineUI();
        BoneListUI();
        MeshUI();
        quaternion.Load(XMQuaternionRotationRollPitchYawFromVector(rotation.XMV()));
        model->SetTransformation(scale, quaternion, translation);
        model->UpdateTransform();
    }
    plane->UpdateTransform();


}

/*-------------------------------------------------GUI TransformUI()---------------------------------------------------------*/

void GUI::TransformUI()
{
    ImGui::Begin("Transform");
    ImGui::InputFloat3("Scale : ", &scale.x);
    ImGui::InputFloat3("Rotation : ", &rotation.x);
    ImGui::InputFloat3("Translation : ", &translation.x);



    ImGui::End();
}

/*-------------------------------------------------GUI AnimationUI()---------------------------------------------------------*/

void GUI::AnimationUI()
{


    ImGui::Begin("Animation");
    std::vector<MODEL_RESOURCES::ANIMATION>&anims = model->Resource()->Animations;
    ImGui::ListBoxHeader("Animations");
    int ind{};
    for (auto& a : anims)
    {
        bool s{};
        if (ImGui::Selectable(a.Name.c_str(), &s)) {
            selected_anim = ind;
            break;
        }
        ++ind;

    }

    ImGui::ListBoxFooter();

    if (selected_anim > -1)
        model->SetTake(selected_anim);


    if (ImGui::Button("Delete Animation") || INPUTMANAGER::Instance()->Keyboard()->Triggered(VK_DELETE))
    {
        anims.erase(anims.begin() + selected_anim);
        selected_anim = 0;
        model->SetTake(0);
        //model->SetLastTake(0);
        ImGui::End();
        return;
    }



    ImGui::Checkbox("Play Animation ?", &playAnim);
    if (!playAnim)
        model->PauseAnim();
    else
        model->ResumeAnim();

    IMGUI::Instance()->InputText("New Animation path", &animation_path);
    if (ImGui::Button("Insert"))
    {
        std::filesystem::path full_name(animation_path);
        std::filesystem::path name(full_name.filename());
        full_name.replace_extension("");
        model->Resource()->AddAnimation(animation_path, full_name.string());
    }
    IMGUI::Instance()->InputText(std::string("Rename Animation"), &anim_name);
    if (ImGui::Button("Rename"))
    {
        anims[selected_anim].Name = anim_name;
    }

    float rate{};
    ImGui::DragFloat("Sampling Rate", &anims[selected_anim].SamplingRate, 0.1f, 0.0f, 120.0f);


    ImGui::End();
}

/*-------------------------------------------------GUI MaterialUI()---------------------------------------------------------*/

void GUI::MaterialUI()
{
    ImGui::Begin("Materials");

    ImGui::ListBoxHeader("Materials");

    for (auto& m : model->Resource()->Materials)
    {
        bool s{};
        if (ImGui::Selectable(m.second.name.c_str(), &s))
        {
            m_selected_item = m.first;
            break;
        }
        
    }

    ImGui::ListBoxFooter();

    if (m_selected_item < 0)
    {
        ImGui::End();
        return;
    }
    for (auto& t : model->Resource()->Materials.find(m_selected_item)->second.texture_path)
    {
        ImGui::Text(t.c_str());
    }



    MODEL_RESOURCES::MATERIAL& m{ model->Resource()->Materials.find(m_selected_item)->second };
    int ind{};
    if (ImGui::BeginCombo("Material Type", types[s_type].c_str()))
    {
        for (auto& t : types)
        {
            bool s{};
            if (ImGui::Selectable(t.c_str(), &s))
            {
                s_type = ind;
                break;
            }
            ++ind;
        }


        ImGui::EndCombo();
    }
    IMGUI::Instance()->InputText("Material File", &material_path);
    if (ImGui::Button("Insert Material"))
    {
        std::filesystem::path path(model_path);
        std::filesystem::path model_name = path.filename().string();
        model_name.replace_extension("");
        model->Resource()->AddMaterial(material_path, model_name.string(), (MODEL_RESOURCES::MATERIAL_TYPE)s_type, model->Resource()->Materials.find(m_selected_item)->second);
    }

    //if (wire)
    //    DirectX11::Instance()->DeviceContext()->RSSetState(RasterizerManager::Instance()->Retrieve("Wireframe").get()->Rasterizer().Get());
    //else
    //    DirectX11::Instance()->DeviceContext()->RSSetState(RasterizerManager::Instance()->Retrieve("Default").get()->Rasterizer().Get());



    //if (ImGui::BeginCombo("Materials : ", mats.find(m_selected_item)->second.name.c_str()))
    //{
    //    for (int ind = 0; ind < UIDs.size(); ++ind)
    //    {
    //        bool selected{ m_selected_item == UIDs[0] };
    //        if (ImGui::Selectable(m_names[ind].c_str(), &selected))
    //        {
    //            m_selected_item = UIDs[ind];
    //        }

    //    }
    //    ImGui::EndCombo();
    //}



    

    ImGui::End();
}

/*-------------------------------------------------GUI TimelineUI()---------------------------------------------------------*/

void GUI::TimelineUI()
{
    if (!model)
        return;
    MODEL_RESOURCES::ANIMATION& a{ model->Resource()->Animations.at(selected_anim) };
    ImGui::Begin("Animation Timeline");
    std::string t{ "Animation Name : " };
    t += a.Name;
    ImGui::Text(t.c_str());
    ImGui::DragInt("Frames : ", &model->cur_Keyframe, 1.0f, 0, a.Keyframes.size() - 1);
    ImGui::End();
}

/*-------------------------------------------------GUI MeshUI()---------------------------------------------------------*/

void GUI::MeshUI()
{
    if (!model)
        return;
    ImGui::Begin("Meshes");
    for (auto& m : model->Resource()->Meshes)
    {
        ImGui::Text(m.Name.c_str());

    }
    ImGui::End();
}


/*-------------------------------------------------GUI Render()---------------------------------------------------------*/

void GUI::Render()
{
    plane->Render();
    if (hr != S_OK)
        return;
    model->Render();
    model->RenderWireframe();
    //ImGui::Begin("Collision");
    //ImGui::Checkbox("Colliding ? ", &colliding);
    //ImGui::End();

}

/*-------------------------------------------------GUI Finalize()---------------------------------------------------------*/

void GUI::Finalize()
{
    //IMGUI::Instance()->End();
}

/*-------------------------------------------------GUI Select()---------------------------------------------------------*/

void GUI::Select()
{
    if (!INPUTMANAGER::Instance()->Keyboard()->Held(VK_CONTROL))
    {
        return;
    }
    if (hr != S_OK)
        return;
    Vector3 cur_pos;
        cur_pos.x = INPUTMANAGER::Instance()->Mouse()->fPosition().x;
        cur_pos.y = INPUTMANAGER::Instance()->Mouse()->fPosition().y;
    D3D11_VIEWPORT vp;
    UINT num{ 1 };
    DirectX11::Instance()->DeviceContext()->RSGetViewports(&num, &vp);
    XMVECTOR Near, Far;
    cur_pos.z = 0;

    Near = XMVector3Unproject(cur_pos.XMV(), vp.TopLeftX, vp.TopLeftY, vp.Width, vp.Height, vp.MinDepth, vp.MaxDepth, DirectX11::Instance()->ProjectionMatrix(), Camera::Instance()->ViewMatrix(), model->TransformMatrix());
    cur_pos.z = 1.0f;
    Far = XMVector3Unproject(cur_pos.XMV(), vp.TopLeftX, vp.TopLeftY, vp.Width, vp.Height, vp.MinDepth, vp.MaxDepth, DirectX11::Instance()->ProjectionMatrix(), Camera::Instance()->ViewMatrix(), model->TransformMatrix());

    Vector3 N, F;
    N.Load(Near);
    F.Load(Far);



    COLLIDERS::RAYCASTDATA rcd{};


    if (INPUTMANAGER::Instance()->Mouse()->LButton().Triggered())
    {
        if (COLLIDERS::RayCast(N, F, model.get(), rcd))
        {
            int a = 0;
            colliding = true;
            selected = model;
        }
        else {
            colliding = false;
            selected.reset();
        }
    }
}

/*-------------------------------------------------GUI Reset()---------------------------------------------------------*/

void GUI::Reset()
{
    //material_path = model_path = animation_path = created_anim_name = created_path = "";
    m_selected_item = -1;
    s_type = selected_anim = {};
}

/*-------------------------------------------------GUI BoneListUI()---------------------------------------------------------*/

void GUI::BoneListUI()
{
    if (ImGui::Begin("Bones"));
    {
        for (auto& m : model->Resource()->Meshes)
        {
            if (ImGui::TreeNode(m.Name.c_str()))
            {
                for (auto& b : m.Bind_Pose.Bones)
                {
                    ImGui::Text(b.Name.c_str());
                    
                }
                ImGui::TreePop();
            }
        }
        ImGui::End();
    }
}
