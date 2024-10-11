#include "File.h"
#include "util.h"

#include <windows.h>
#include <string>
#include <shobjidl.h>

void load_file(std::string filename, std::vector<std::string>* shader_paths, std::vector<std::vector<int>>* shader_indices, std::vector<Model*>* scene, size_t sceneSize, Camera* camera)
{

    std::ifstream f{ "res/data/user/" + filename };
    json j = json::parse(f);

    for (size_t i = 0; i < sceneSize; ++i)
    {
        (*scene)[i]->translationVec = glm::vec3(j["SceneInfo"]["Objects"][(*scene)[i]->UUID.c_str()]["TRANSLATION"][0], j["SceneInfo"]["Objects"][(*scene)[i]->UUID.c_str()]["TRANSLATION"][1], j["SceneInfo"]["Objects"][(*scene)[i]->UUID.c_str()]["TRANSLATION"][2]);
        (*scene)[i]->rotationVec = glm::vec3(j["SceneInfo"]["Objects"][(*scene)[i]->UUID.c_str()]["ROTATION"][0], j["SceneInfo"]["Objects"][(*scene)[i]->UUID.c_str()]["ROTATION"][1], j["SceneInfo"]["Objects"][(*scene)[i]->UUID.c_str()]["ROTATION"][2]);
        (*scene)[i]->scaleVec = glm::vec3(j["SceneInfo"]["Objects"][(*scene)[i]->UUID.c_str()]["SCALE"][0], j["SceneInfo"]["Objects"][(*scene)[i]->UUID.c_str()]["SCALE"][1], j["SceneInfo"]["Objects"][(*scene)[i]->UUID.c_str()]["SCALE"][2]);
    }

    camera->Position = glm::vec3(j["SceneInfo"]["CameraInfo"]["CameraPos"][0], j["SceneInfo"]["CameraInfo"]["CameraPos"][1], j["SceneInfo"]["CameraInfo"]["CameraPos"][2]);
    camera->Orientation = glm::vec3(j["SceneInfo"]["CameraInfo"]["CameraOrientation"][0], j["SceneInfo"]["CameraInfo"]["CameraOrientation"][1], j["SceneInfo"]["CameraInfo"]["CameraOrientation"][2]);
    camera->lightPos = glm::vec3(j["SceneInfo"]["LightInfo"]["LightPos"][0], j["SceneInfo"]["LightInfo"]["LightPos"][1], j["SceneInfo"]["LightInfo"]["LightPos"][2]);
    camera->lightColor = glm::vec3(j["SceneInfo"]["LightInfo"]["LightColor"][0], j["SceneInfo"]["LightInfo"]["LightColor"][1], j["SceneInfo"]["LightInfo"]["LightColor"][2]);

    (*shader_paths) = j["SceneInfo"]["GraphicsPipelines"]["ShaderPaths"];
    (*shader_indices) = j["SceneInfo"]["GraphicsPipelines"]["ShaderIndices"];
    return;
}

void delete_file(std::string fileName)
{
    std::string finalString = "res/data/user/" + fileName;
    if (std::remove(finalString.c_str()) == 0)
    {
        printf("Succesfully deleted file: %s", fileName.c_str());
    }
}


void write_file(std::vector<Model*> scene, std::string scene_path, std::vector<std::string>* shader_paths, std::vector<std::vector<int>>* shader_indices, Camera* camera)
{
    bool willNotReturn = false;
    std::vector<std::string> ids;
    ids.resize(scene.size());
    for (size_t i = 0; i < scene.size(); ++i)
    {

        if (scene[i]->UUID.empty())
        {
            std::string wrn = std::string("Empty UUID cannot serialize model with index: ") + std::to_string(i) + "\n";

            tlog::warning(wrn);
            willNotReturn = true;

        }
        for (auto& id : ids)
        {
            if (!strcmp(scene[i]->UUID.c_str(), id.c_str()))
            {
                std::string wrn = std::string("UUID of model with index: ") + std::to_string(i) + " already exists\n";
                tlog::warning(wrn);
                willNotReturn = true;
            }

        }
        ids[i] = scene[i]->UUID;

    }
    if (willNotReturn)
        return;
    std::ofstream f("res/data/user/" + scene_path);
    json j;

    j["SceneSize"] = scene.size();

    j["SceneInfo"]["CameraInfo"]["CameraPos"] = { json::number_float_t(camera->Position.x), json::number_float_t(camera->Position.y), json::number_float_t(camera->Position.z) };
    j["SceneInfo"]["CameraInfo"]["CameraOrientation"] = { json::number_float_t(camera->Orientation.x), json::number_float_t(camera->Orientation.y), json::number_float_t(camera->Orientation.z) };
    j["SceneInfo"]["LightInfo"]["LightPos"] = { json::number_float_t(camera->lightPos.x), json::number_float_t(camera->lightPos.y), json::number_float_t(camera->lightPos.z) };
    j["SceneInfo"]["LightInfo"]["LightColor"] = { json::number_float_t(camera->lightColor.x), json::number_float_t(camera->lightColor.y), json::number_float_t(camera->lightColor.z) };

    for (auto& shader_path : *shader_paths)
    {
        j["SceneInfo"]["GraphicsPipelines"]["ShaderPaths"].push_back(shader_path);
    }
    for (auto& shader_index : *shader_indices)
    {
        j["SceneInfo"]["GraphicsPipelines"]["ShaderIndices"].push_back(shader_index);
    }

    for (size_t i = 0; i < scene.size(); i++)
    {
        j["SceneInfo"]["Objects"][scene[i]->UUID.c_str()]["ModelPath"] = json::string_t(scene[i]->MODEL_PATH);
        j["SceneInfo"]["Objects"][scene[i]->UUID.c_str()]["TexturePath"] = json::string_t(scene[i]->TEXTURE_PATH);
        j["SceneInfo"]["Objects"][scene[i]->UUID.c_str()]["NormalPath"] = json::string_t(scene[i]->NORMAL_PATH);
        j["SceneInfo"]["Objects"][scene[i]->UUID.c_str()]["GraphicsPipeline"] = json::number_integer_t(scene[i]->pipelineIndex);
        j["SceneInfo"]["Objects"][scene[i]->UUID.c_str()]["TRANSLATION"] = { json::number_float_t(scene[i]->translationVec.x), json::number_float_t(scene[i]->translationVec.y), json::number_float_t(scene[i]->translationVec.z) };
        j["SceneInfo"]["Objects"][scene[i]->UUID.c_str()]["ROTATION"] = { json::number_float_t(scene[i]->rotationVec.x), json::number_float_t(scene[i]->rotationVec.y), json::number_float_t(scene[i]->rotationVec.z) };
        j["SceneInfo"]["Objects"][scene[i]->UUID.c_str()]["SCALE"] = { json::number_float_t(scene[i]->scaleVec.x), json::number_float_t(scene[i]->scaleVec.y), json::number_float_t(scene[i]->scaleVec.z) };
    }

    f << std::setw(4) << j << std::endl;
}

void find_files(std::vector<std::string>* scene_paths, std::string sceneDirectory, std::string fileExtension)
{

    scene_paths->resize(0);
    for (const auto& entry : std::filesystem::directory_iterator(sceneDirectory))
    {
        std::filesystem::path outfilename = entry.path();
        if (outfilename.extension() == fileExtension)
        {
            std::string outfilename_str = outfilename.string();
            scene_paths->push_back(util::clear_slash(outfilename_str));
        }
    }
}

void load_scene(std::string scene_path, std::vector<Model*>* scene, size_t* sceneSize)
{
    scene->resize(0);
    std::ifstream f{ "res/data/user/" + scene_path };

    json j = json::parse(f);
    *sceneSize = j["SceneSize"];
    std::vector<std::string> UUIDs;
    for (auto it = j["SceneInfo"]["Objects"].items().begin(); it != j["SceneInfo"]["Objects"].items().end(); ++it)
    {
        auto elem = it.key();
        UUIDs.push_back(elem);
    }

    for (size_t i = 0; i < *sceneSize; ++i)
    {
        std::string s1 = j["SceneInfo"]["Objects"][UUIDs[i]]["ModelPath"];
        std::string s2 = j["SceneInfo"]["Objects"][UUIDs[i]]["TexturePath"];

        Model* cModel = new Model;
        init_model(cModel, s1.c_str(), s2.c_str());
        cModel->NORMAL_PATH = j["SceneInfo"]["Objects"][UUIDs[i]]["NormalPath"];
        cModel->UUID = UUIDs[i];
        cModel->pipelineIndex = j["SceneInfo"]["Objects"][UUIDs[i]]["GraphicsPipeline"];
        load_model(cModel);
        scene->push_back(cModel);
    }
}



std::string pick_file(EXTENSION ext)
{
    /*
    * GOD I HATE THE WINDOWS API
    */

    // Create File Object Instance
    HRESULT f_SysHr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(f_SysHr))
        return "";

    // Create FileOpenDialog Object
    IFileOpenDialog* f_FileSystem;
    f_SysHr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&f_FileSystem));
    if (FAILED(f_SysHr)) {
        CoUninitialize();
        return "";
    }

    // Set file types to filter (example for .txt and .cpp files)
    /*
    const COMDLG_FILTERSPEC fileTypes[] =
    {
        { L"Image File (*.png; *.jpg)", L"*.png; *.jpg" },
        { L"Wavefront Object File (*.obj)", L"*.obj" },
        { L"Compiled SPIR-V Shader (*.spv)", L"*.spv"},
        { L"JSON Files (*.json)", L"*.json"},
        { L"All Files (*.*)", L"*.*"}
    };*/

    COMDLG_FILTERSPEC fileTypes[2] = {};
    switch (ext)
    {
        case EXTENSIONS_MODELS:
        {
            fileTypes[0] = { L"Wavefront Object File (*.obj)", L"*.obj" };
            fileTypes[1] = { L"All Files (*.*)", L"*.*" };
        }break;
        case EXTENSIONS_IMAGES:
        {
            fileTypes[0] = { L"Image File (*.png; *.jpg)", L"*.png; *.jpg" };
            fileTypes[1] = { L"All Files (*.*)", L"*.*" };
        }break;
        case EXTENSIONS_SHADERS:
        {
            fileTypes[0] = { L"Compiled SPIR-V Shader (*.spv)", L"*.spv" };
            fileTypes[1] = { L"All Files (*.*)", L"*.*" };
        }break;
        case EXTENSIONS_SERIALIZATION:
        {
            fileTypes[0] = { L"Serialized Data (*.json; *.dta)", L"*.json; *.dta" };
            fileTypes[1] = { L"All Files (*.*)", L"*.*" };
        }break;
    }

    f_FileSystem->SetFileTypes(ARRAYSIZE(fileTypes), fileTypes);
    f_FileSystem->SetFileTypeIndex(1); // Set default selection to the first item in the filter
    f_FileSystem->SetDefaultExtension(L"txt"); // Default file extension


    // Show Open file Dialog Window
    f_SysHr = f_FileSystem->Show(NULL);
    if (FAILED(f_SysHr)) {
        f_FileSystem->Release();
        CoUninitialize();
        return "";
    }

    // Retrieve File name from selected file
    IShellItem* f_Files;
    f_SysHr = f_FileSystem->GetResult(&f_Files);
    if (FAILED(f_SysHr)) {
        f_FileSystem->Release();
        CoUninitialize();
        return "";
    }

    // Store and convert
    PWSTR f_Path;
    f_SysHr = f_Files->GetDisplayName(SIGDN_FILESYSPATH, &f_Path);
    if (FAILED(f_SysHr)) {
        f_Files->Release();
        f_FileSystem->Release();
        CoUninitialize();
        return "";
    }
    std::string sFilePath;
    std::string res;

    std::wstring path(f_Path);
    std::string c(path.begin(), path.end());
    sFilePath = c;

    //  FORMAT STRING FOR EXECUTABLE NAME
    const size_t slash = sFilePath.find_last_of("/\\");
    res = sFilePath.substr(slash + 1);

    //  SUCCESS, CLEAN UP
    CoTaskMemFree(f_Path);
    f_Files->Release();
    f_FileSystem->Release();
    CoUninitialize();
    return res;
}