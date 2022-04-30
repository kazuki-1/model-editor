#pragma once
#include "Engine/IMGUI.h"
#include "Engine/Singleton.h"

class GUI : public Singleton<GUI>
{


    friend class MODEL;
    std::string model_path{ "" }, animation_path{ "" }, created_path{ "" }, created_anim_name{ "" }, material_path{ "" };
    bool fbx, mrs;
    HRESULT hr{ E_FAIL };


public:

    void Initialize();
    void Execute();
    void Render();
    void TransformUI();
    void MaterialUI();
    void AnimationUI();
    void TimelineUI();
    void MeshUI();
    void Reset();
    void BoneListUI();
    void NodeList();

    void Finalize();
    void Select();


};


