#pragma once

// CORE
#include "Triangle.h"

// STD
#include <vector>
#include <string>

// TINYOBJ
#include "tinyobjloader/tiny_obj_loader.h"

class Mesh
{
public:
	Mesh(const char* filepath);
	~Mesh();

	const std::vector<Triangle>& GetTriangles() const { return triangles; }
private:
	std::vector<Triangle> triangles;
	tinyobj::attrib_t m_attributes;
	std::vector<tinyobj::shape_t> m_shapes;
	std::vector<tinyobj::material_t> m_materials;
	std::string m_warning;
	std::string m_error;
};