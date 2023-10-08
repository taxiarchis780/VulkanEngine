#ifndef __UTIL_H__
#define __UTIL_H__

#include <fstream>
#include <vector>
#include <Engine.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/ext/matrix_float4x4_precision.hpp>
#include <glm/gtx/hash.hpp>

std::string clear_slash(std::string const& path_of_file, std::string const& d_slash = "/\\")
{
	size_t index_of_slash = path_of_file.find_last_of(d_slash);
	std::string file_name = path_of_file.substr(index_of_slash + 1);
	return file_name;
}


static std::vector<char> readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("ERROR: failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}


bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale)
{
	// From glm::decompose in matrix_decompose.inl

	using namespace glm;
	using T = float;

	highp_mat4 LocalMatrix(transform);

	// Normalize the matrix.
	if (epsilonEqual(LocalMatrix[3][3], static_cast<float>(0), epsilon<T>()))
		return false;

	// First, isolate perspective.  This is the messiest.
	if (
		epsilonNotEqual(LocalMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
		epsilonNotEqual(LocalMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
		epsilonNotEqual(LocalMatrix[2][3], static_cast<T>(0), epsilon<T>()))
	{
		// Clear the perspective partition
		LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<T>(0);
		LocalMatrix[3][3] = static_cast<T>(1);
	}

	// Next take care of translation (easy).
	translation = highp_vec3(LocalMatrix[3]);
	LocalMatrix[3] = highp_vec4(0, 0, 0, LocalMatrix[3].w);

	highp_vec3 Row[3] = {highp_vec3(0.0f), highp_vec3(0.0f) ,highp_vec3(0.0f) };

	// Now get scale and shear.
	for (length_t i = 0; i < 3; ++i)
		for (length_t j = 0; j < 3; ++j)
			Row[i][j] = LocalMatrix[i][j];

	// Compute X scale factor and normalize first row.
	
	scale.x = length(Row[0]);
	Row[0] = detail::scale(Row[0], static_cast<T>(1));
	scale.y = length(Row[1]);
	Row[1] = detail::scale(Row[1], static_cast<T>(1));
	scale.z = length(Row[2]);
	Row[2] = detail::scale(Row[2], static_cast<T>(1));

	// At this point, the matrix (in rows[]) is orthonormal.
	// Check for a coordinate system flip.  If the determinant
	// is -1, then negate the matrix and the scaling factors.
#if 0
	highp_vec3 Pdum3[3] = {};
	Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
	if (dot(Row[0], Pdum3) < 0)
	{
		for (length_t i = 0; i < 3; i++)
		{
			scale[i] *= static_cast<T>(-1);
			Row[i] *= static_cast<T>(-1);
		}
	}
#endif

	rotation.y = asin(-Row[0][2]);
	if (cos(rotation.y) != 0) {
		rotation.x = atan2(Row[1][2], Row[2][2]);
		rotation.z = atan2(Row[0][1], Row[0][0]);
	}
	else {
		rotation.x = atan2(-Row[2][0], Row[1][1]);
		rotation.z = 0;
	}


	return true;
}

// https://stackoverflow.com/questions/9543715/generating-human-readable-usable-short-but-unique-ids
void GenerateUUID(Model* model, int length ,bool useBase62)
{
	const char* baseChars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	int baseLength = useBase62 ? 62 : 36;

	std::stringstream ss;
	std::srand(std::time(nullptr));

	for (int i = 0; i < length; i++)
		ss << baseChars[std::rand() % baseLength];

	model->UUID = ss.str();
}

glm::vec3 rayCast(double xpos, double ypos, double WIDTH, double HEIGHT, glm::mat4 projection, glm::mat4 view) {
	// converts a position from the 2d xpos, ypos to a normalized 3d direction
	float x = (2.0f * xpos) / WIDTH - 1.0f;
	float y = 1.0f - (2.0f * ypos) / HEIGHT;
	float z = 1.0f;
	glm::vec3 ray_nds = glm::vec3(x, y, z);
	glm::vec4 ray_clip = glm::vec4(ray_nds.x, ray_nds.y, -1.0f, 1.0f);
	// eye space to clip we would multiply by projection so
	// clip space to eye space is the inverse projection
	glm::vec4 ray_eye = inverse(projection) * ray_clip;
	// convert point to forwards
	ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f);
	// world space to eye space is usually multiply by view so
	// eye space to world space is inverse view
	glm::vec4 inv_ray_wor = (inverse(view) * ray_eye);
	glm::vec3 ray_wor = glm::vec3(inv_ray_wor.x, inv_ray_wor.y, inv_ray_wor.z);
	ray_wor = glm::normalize(ray_wor);
	
	return ray_wor;
}


#endif