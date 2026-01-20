#pragma once
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <string>
#include <vector>

#include "ModelIO/MDLDefines.hpp"
#include "ModelIO/ModelIO.hpp"
#include "BufferUtilites.hpp"
#include "Material.hpp"

class Submesh {
public:
    Submesh(MTL::PrimitiveType primitiveType,
            BufferView indexBuffer,
            MTL::IndexType indexType,
            NS::UInteger indexCount,
            int materialIndex);
    
    const MTL::PrimitiveType primitiveType;
    const BufferView indexBuffer;
    const MTL::IndexType indexType;
    const NS::UInteger indexCount;
    int materialIndex;
};
class Mesh {
public:
    Mesh(std::string name,
         std::vector<BufferView> vertexBuffers,
         NS::UInteger vertexCount,
         NS::SharedPtr<MDL::VertexDescriptor> vertexDescriptor,
         std::vector<Submesh> submeshes,
         std::vector<Material> materials);
    
    std::string name;
    const std::vector<BufferView> vertexBuffers;
    const NS::UInteger vertexCount;
    const NS::SharedPtr<MDL::VertexDescriptor> vertexDescriptor;
    const std::vector<Submesh> submeshes;
    std::vector<Material> materials;
};
