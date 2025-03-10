#include "util.h"




namespace util
{
	std::string clear_slash(std::string const& path_of_file, std::string const& d_slash)
	{
		size_t index_of_slash = path_of_file.find_last_of(d_slash);
		std::string file_name = path_of_file.substr(index_of_slash + 1);
		return file_name;
	}

	std::vector<char> readFile(const std::string& filename)
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

	

	// https://stackoverflow.com/questions/9543715/generating-human-readable-usable-short-but-unique-ids
	void GenerateUUID(Model* model, int length, bool useBase62)
	{
		const char* baseChars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
		int baseLength = useBase62 ? 62 : 36;

		std::stringstream ss;
		std::srand(std::time(nullptr));

		for (int i = 0; i < length; i++)
			ss << baseChars[std::rand() % baseLength];

		model->UUID = ss.str();
	}

	static physx::PxMat44 glmMat4ToPhysxMat4(const glm::mat4& mat4)
	{
		physx::PxMat44 newMat = {};

		newMat[0][0] = mat4[0][0];
		newMat[0][1] = mat4[0][1];
		newMat[0][2] = mat4[0][2];
		newMat[0][3] = mat4[0][3];

		newMat[1][0] = mat4[1][0];
		newMat[1][1] = mat4[1][1];
		newMat[1][2] = mat4[1][2];
		newMat[1][3] = mat4[1][3];

		newMat[2][0] = mat4[2][0];
		newMat[2][1] = mat4[2][1];
		newMat[2][2] = mat4[2][2];
		newMat[2][3] = mat4[2][3];

		newMat[3][0] = mat4[3][0];
		newMat[3][1] = mat4[3][1];
		newMat[3][2] = mat4[3][2];
		newMat[3][3] = mat4[3][3];


		return newMat;
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

		highp_vec3 Row[3] = { highp_vec3(0.0f), highp_vec3(0.0f) ,highp_vec3(0.0f) };

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


#if 0
		// At this point, the matrix (in rows[]) is orthonormal.
		// Check for a coordinate system flip.  If the determinant
		// is -1, then negate the matrix and the scaling factors.
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


	int findHighestElementIfNotUnique(const std::vector<int>& totalStrength)
	{
		if (totalStrength.empty())
		{
			return -1;
		}

		int max_value = *std::max_element(totalStrength.begin(), totalStrength.end());
		int count_max = std::count(totalStrength.begin(), totalStrength.end(), max_value);

		if (count_max == 1)
		{
			// The highest integer is unique
			auto it = std::find(totalStrength.begin(), totalStrength.end(), max_value);
			return std::distance(totalStrength.begin(), it);

		}
		else {
			return -1;
		}
	}

	glm::vec2 worldToScreen(glm::vec3 pos, glm::mat4 proj, glm::mat4 view)
	{
		glm::vec4 clipSpacePos = proj * (view * glm::vec4(pos, 1.0));
		glm::vec3 ndcSpacePos;
		glm::vec2 windowSpacePos;
		if (!clipSpacePos.w) {
			printf("Panik!");
			throw std::runtime_error("ClipSpacePos.w was 0!! Cannot divide by 0!");
		}

		ndcSpacePos.x = clipSpacePos.x / clipSpacePos.w;
		ndcSpacePos.y = clipSpacePos.y / clipSpacePos.w;
		ndcSpacePos.z = clipSpacePos.z / clipSpacePos.w;

		windowSpacePos.x = ((ndcSpacePos.x + 1.0) / 2.0);
		windowSpacePos.y = 1.0f - ((ndcSpacePos.y + 1.0) / 2.0); // I want the screen coordinates to be on the first quadrant

		return windowSpacePos;

	}

	bool isInsideQuadrilateral(const glm::vec2& click, const glm::vec2& A, const glm::vec2& B,
		const glm::vec2& C, const glm::vec2& D) {
		// Define the vertices of the quadrilateral
		glm::vec2 vertices[4] = { A, B, C, D };

		// Compute vectors from click point to each vertex
		glm::vec2 vectors[4];
		for (int i = 0; i < 4; ++i) {
			vectors[i] = vertices[i] - click;
		}

		// Compute cross products
		float crossProducts[4];
		for (int i = 0; i < 4; ++i) {
			int next = (i + 1) % 4;
			crossProducts[i] = glm::cross(glm::vec3(vectors[i], 0.0f), glm::vec3(vectors[next], 0.0f)).z;
		}

		// Check if all cross products have the same sign
		bool signCheck = (crossProducts[0] >= 0 && crossProducts[1] >= 0 && crossProducts[2] >= 0 && crossProducts[3] >= 0) ||
			(crossProducts[0] <= 0 && crossProducts[1] <= 0 && crossProducts[2] <= 0 && crossProducts[3] <= 0);

		return signCheck;
	}
}