#include "Mesh.h"

// PCR
#include "Utils.h"

Mesh::Mesh(const char* filepath)
{
	if (!tinyobj::LoadObj(&m_attributes, &m_shapes, &m_materials, &m_warning, &m_error, filepath)) 
	{
        std::string message(m_warning + m_error);
		Utils::ThrowFatalError(message.c_str());
	}

    for (const auto& shape : m_shapes)
    {
        for (size_t f = 0; f < shape.mesh.indices.size(); f += 3)
        {
            tinyobj::index_t i0 = shape.mesh.indices[f + 0];
            tinyobj::index_t i1 = shape.mesh.indices[f + 1];
            tinyobj::index_t i2 = shape.mesh.indices[f + 2];

            triangles.push_back
            ({
                glm::vec4(m_attributes.vertices[3 * i0.vertex_index + 0], m_attributes.vertices[3 * i0.vertex_index + 1], m_attributes.vertices[3 * i0.vertex_index + 2], 0),
                glm::vec4(m_attributes.vertices[3 * i1.vertex_index + 0], m_attributes.vertices[3 * i1.vertex_index + 1], m_attributes.vertices[3 * i1.vertex_index + 2], 0),
                glm::vec4(m_attributes.vertices[3 * i2.vertex_index + 0], m_attributes.vertices[3 * i2.vertex_index + 1], m_attributes.vertices[3 * i2.vertex_index + 2], 0)
            });
        }
    }

    if (triangles.empty())
        Utils::ThrowFatalError("No triangles loaded from OBJ.");
}

Mesh::~Mesh(){}
