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
char anim_name[256] = "";
std::string model_name;
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

    if (ImGui::Button("Open model") )
    {
            browser->Open();
            browser->SetTitle("Open Model File");
            browser->SetTypeFilters({ ".fbx", ".mrs", ".obj", ".*" });
            isEmpty = fileOpen = true;

    }

    if (fileOpen)
    {
        browser->Display();
        if (browser->HasSelected())
        {
            model_path = browser->GetSelected().string();
            std::filesystem::path path(model_path);
            model_name = path.filename().string();
            path.replace_extension("");

            fileOpen = false;
            browser->Close();
            if (hr == S_OK) {
                model = std::make_shared<MODEL>();
            }
            hr = model->Initialize(model_path);
            model->SetTake(0);
            model->SetFrame(0);
            Reset();

        }
    }

    ImGui::FileBrowser* creator{IMGUI::Instance()->FileCreator()};
    static bool fileCreate{};
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

        IMGUI::Instance()->CreatePopup("Model Recreated! Check ./Generated for file", &created_popup);
    }

    ImGui::Separator();

    if (ImGui::Button("Reset Camera"))
    {
        Camera::Instance()->ResetToTarget({ 0, 0, 0 });
    }


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
    ImGui::FileBrowser* browser{ IMGUI::Instance()->FileBrowser() };

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




    static bool fileOpenA{};

    //IMGUI::Instance()->InputText("New Animation path", &animation_path);
    if (ImGui::Button("Load animation"))
    {
        browser->Open();
        browser->SetTitle("Open animation");
        browser->SetTypeFilters({ ".fbx", ".*" });
        fileOpenA = true;

    }

    if (fileOpenA)
    {
        browser->Display();
        if (browser->HasSelected())
        {
            animation_path = browser->GetSelected().string();
            std::filesystem::path full_name(animation_path);
            std::filesystem::path name(full_name.filename());
            full_name.replace_extension("");
            model->Resource()->AddAnimation(animation_path, full_name.string());

            fileOpenA = false;
            browser->Close();
        }
    }



    //IMGUI::Instance()->InputText(std::string("Rename Animation"), &anim_name);
    ImGui::InputText("Rename Animation", anim_name, 256);
    if (ImGui::Button("Rename"))
    {
        anims[selected_anim].Name = std::string(anim_name);
    }

    float rate{};
    ImGui::DragFloat("Sampling Rate", &anims[selected_anim].SamplingRate, 0.1f, 0.0f, 120.0f);


    ImGui::End();
}

/*-------------------------------------------------GUI MaterialUI()---------------------------------------------------------*/

void GUI::MaterialUI()
{
    ImGui::FileBrowser* browser{ IMGUI::Instance()->FileBrowser() };



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

    static bool openFileM{};

    if (ImGui::Button("Load Texture"))
    {
        browser->Open();
        browser->SetTitle("Open texture file");
        browser->SetTypeFilters({ ".png", ".jpg", ".tif", ".*" });
        openFileM = true;
        model->Resource()->AddMaterial(material_path, model_name, (MODEL_RESOURCES::MATERIAL_TYPE)s_type, model->Resource()->Materials.find(m_selected_item)->second);
    }

    if (openFileM)
    {
        browser->Display();
        if (browser->HasSelected())
        {
            material_path = browser->GetSelected().string();
            model->Resource()->AddMaterial(material_path, model_name, (MODEL_RESOURCES::MATERIAL_TYPE)s_type, model->Resource()->Materials.find(m_selected_item)->second);
            browser->Close();
            openFileM = false;
        }
    }



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
    ImGui::SliderInt("Frames : ", &model->cur_Keyframe, 0, a.Keyframes.size() - 1);
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

void OutputChild(int node_index)
{
    MODEL_RESOURCES::SCENE::NODE node = model->Resource()->Scenes.Nodes[node_index];
        int parent_node_index = node.p_Index;
    if (parent_node_index != -1) {
        ImGui::Text(node.Name.c_str());
        node_index = parent_node_index;
        OutputChild(node_index);
    }
    return;

}



void GUI::NodeList()
{
    //if (ImGui::Begin("Nodes"))
    //{
    //    for (auto& node : model->resource->Scenes.Nodes)
    //    {
    //        if (ImGui::TreeNode(node.Name.c_str()))
    //        {
    //            for (auto& child_node : model->resource->Scenes.Nodes)
    //            {
    //                OutputChild(child_node.);
    //            }
    //        }
    //    }
    //}
}