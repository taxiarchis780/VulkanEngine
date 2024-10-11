#ifndef __FILE_H__
#define __FILE_H__

#include <string>
#include <fstream>

#include <json.hpp>
using json = nlohmann::ordered_json;


#include "Camera.h"

enum EXTENSION {
    EXTENSIONS_MODELS,
    EXTENSIONS_IMAGES,
    EXTENSIONS_SHADERS,
    EXTENSIONS_SERIALIZATION
};


void delete_file(std::string fileName);

void load_file(std::string filename, std::vector<std::string>* shader_paths, std::vector<std::vector<int>>* shader_indices, std::vector<Model*>* scene, size_t sceneSize, Camera* camera);

void write_file(std::vector<Model*> scene, std::string scene_path, std::vector<std::string>* shader_paths, std::vector<std::vector<int>>* shader_indices, Camera* camera);

void find_files(std::vector<std::string>* scene_paths, std::string sceneDirectory, std::string fileExtension);

void load_scene(std::string scene_path, std::vector<Model*>* scene, size_t* sceneSize);

std::string pick_file(EXTENSION ext);

#endif