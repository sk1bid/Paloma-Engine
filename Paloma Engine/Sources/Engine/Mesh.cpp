//
//  Mesh.cpp
//  Paloma Engine
//
//  Created by Artem on 19.01.2026.
//

#include "Mesh.hpp"

Submesh::Submesh(MTL::PrimitiveType primitiveType,
                 BufferView indexBuffer,
                 MTL::IndexType indexType,
                 NS::UInteger indexCount,
                 int materialIndex)
: primitiveType(primitiveType)
, indexBuffer(indexBuffer)
, indexType(indexType)
, indexCount(indexCount)
, materialIndex(materialIndex)
{
}

Mesh::Mesh(std::string name,
           std::vector<BufferView> vertexBuffers,
           NS::UInteger vertexCount,
           NS::SharedPtr<MDL::VertexDescriptor> vertexDescriptor,
           std::vector<Submesh> submeshes,
           std::vector<Material> materials)
: name(name)
, vertexBuffers(vertexBuffers)
, vertexCount(vertexCount)
, vertexDescriptor(vertexDescriptor)
, submeshes(submeshes)
, materials(materials)
{
}
