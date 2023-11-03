#define TINYOBJLOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define VMA_IMPLEMENTATION

#include "Renderer.h"

#include "tiny_obj_loader.h"

void keyCallback(
	GLFWwindow *window,
	int key,
	int scancode,
	int action,
	int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(
			window,
			GLFW_TRUE);
	}
}

std::tuple<std::vector<Vertex>, std::vector<uint16_t>> loadObj(const char *filename)
{
	tinyobj::ObjReaderConfig config{};
	config.triangulate = true;
	tinyobj::ObjReader objReader{};
	objReader.ParseFromFile(filename, config);

	const auto attrib = objReader.GetAttrib();
	const auto shapes = objReader.GetShapes();
	const auto materials = objReader.GetMaterials();
	// const std::vector<tinyobj::material_t> &materials = objReader.GetMaterials();

	std::srand(std::time(nullptr));

	std::vector<Vertex> vertices{};
	std::vector<uint16_t> indices{};

	for (int index = 0; index < static_cast<int>(shapes.at(0).mesh.indices.size()); index++)
	{
		const auto &idx = shapes.at(0).mesh.indices.at(index);

		Vertex vertex;
		vertex.pos[0] = attrib.vertices.at(3 * idx.vertex_index);
		vertex.pos[1] = attrib.vertices.at(3 * idx.vertex_index + 1);
		vertex.pos[2] = attrib.vertices.at(3 * idx.vertex_index + 2);
		vertex.color[0] = static_cast<float>(std::rand()) / RAND_MAX;
		vertex.color[1] = static_cast<float>(std::rand()) / RAND_MAX;
		vertex.color[2] = static_cast<float>(std::rand()) / RAND_MAX;
		vertex.texCoords[0] = attrib.texcoords.at(2 * idx.texcoord_index);
		// REMEMBER THE Y AXIS IS FLIPPED IN VULKAN
		vertex.texCoords[1] = 1 - attrib.texcoords.at(2 * idx.texcoord_index + 1);

		indices.push_back(index);
		vertices.push_back(vertex);
	}

	return std::make_tuple(
		vertices,
		indices);
}

int main()
{
	if (!glfwInit())
		throw std::runtime_error("Could not initialize GLFW");

	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow *window = glfwCreateWindow(
		1600,
		900,
		"Testing Vulkan!",
		nullptr,
		nullptr);

	if (!window)
	{
		std::cerr << "Could not open primary window!" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwSetKeyCallback(
		window,
		keyCallback);

	Renderer renderer(window);

	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

	auto objLoadStartTime = std::chrono::high_resolution_clock::now();
	std::tie(
		vertices,
		indices) = loadObj("models/monkey.obj");
	auto objLoadEndTime = std::chrono::high_resolution_clock::now();

	auto elapsedTime = static_cast<std::chrono::duration<float, std::chrono::milliseconds::period>>(objLoadEndTime -
																									objLoadStartTime);

	std::cout << "Took " << elapsedTime.count() << " milliseconds to load OBJ model" << std::endl;

	renderer.uploadVertexData(vertices);
	renderer.uploadIndexData(indices);

	Texture texture = renderer.createTexture("res/monkey.png");

	renderer.updateDescriptorSets(texture);

	renderer.loop();

	renderer.destroy();

	return 0;
}
