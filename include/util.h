#ifndef __UTIL_H__
#define __UTIL_H__

#include <fstream>
#include <vector>
#include <Model.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include "glmIncludes.h"


namespace util
{
	std::string clear_slash(std::string const& path_of_file, std::string const& d_slash = "/\\");
	std::vector<char> readFile(const std::string& filename);
	
	void GenerateUUID(Model* model, int length, bool useBase62);
	static physx::PxMat44 glmMat4ToPhysxMat4(const glm::mat4& mat4);
	bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);

	int findHighestElementIfNotUnique(const std::vector<int>& totalStrength);

	glm::vec2 worldToScreen(glm::vec3 pos, glm::mat4 proj, glm::mat4 view);
	
	bool isInsideQuadrilateral(const glm::vec2& click, const glm::vec2& A, const glm::vec2& B, const glm::vec2& C, const glm::vec2& D);
}




#endif