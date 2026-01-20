/*
 * MTKModel.hpp
 */

#pragma once

#include "MetalKitPrivate.hpp"
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <ModelIO/ModelIO.hpp>

namespace MTK
{

class MeshBufferAllocator : public NS::Referencing<MTK::MeshBufferAllocator, MDL::MeshBufferAllocator>
{
public:
    static class MeshBufferAllocator* alloc();
    class MeshBufferAllocator*        init(MTL::Device* pDevice);

    MTL::Device*                      device() const;
};

class MeshBuffer : public NS::Referencing<MTK::MeshBuffer, MDL::MeshBuffer>
{
public:
    NS::UInteger               length() const;
    class MeshBufferAllocator* allocator() const;
    MDL::MeshBufferZone*       zone() const;
    MTL::Buffer*               buffer() const;
    NS::UInteger               offset() const;
    MDL::MeshBufferType        type() const;
};

class Mesh;

class Submesh : public NS::Referencing<MTK::Submesh>
{
public:
    MTL::PrimitiveType primitiveType() const;
    MTL::IndexType     indexType() const;
    class MeshBuffer*  indexBuffer() const;
    NS::UInteger       indexCount() const;
    class Mesh*        mesh() const;
    NS::String*        name() const;
    void               setName(const NS::String* pName);
};

class Mesh : public NS::Referencing<MTK::Mesh>
{
public:
    static class Mesh*     alloc();
    class Mesh*            init(MDL::Mesh* pMesh, MTL::Device* pDevice, NS::Error** ppError);

    static NS::Array*      newMeshes(MDL::Asset* pAsset, MTL::Device* pDevice, NS::Array** ppSourceMeshes, NS::Error** ppError);

    NS::Array*             vertexBuffers() const;
    MDL::VertexDescriptor* vertexDescriptor() const;
    NS::Array*             submeshes() const;
    NS::UInteger           vertexCount() const;
    NS::String*            name() const;
    void                   setName(const NS::String* pName);
};

// Functions

MTL::VertexDescriptor* MetalVertexDescriptorFromModelIO(MDL::VertexDescriptor* pModelDescriptor);
MTL::VertexDescriptor* MetalVertexDescriptorFromModelIOWithError(MDL::VertexDescriptor* pModelDescriptor, NS::Error** ppError);
MDL::VertexDescriptor* ModelIOVertexDescriptorFromMetal(MTL::VertexDescriptor* pMetalDescriptor);
MDL::VertexDescriptor* ModelIOVertexDescriptorFromMetalWithError(MTL::VertexDescriptor* pMetalDescriptor, NS::Error** ppError);
MTL::VertexFormat      MetalVertexFormatFromModelIO(MDL::VertexFormat vertexFormat);
MDL::VertexFormat      ModelIOVertexFormatFromMetal(MTL::VertexFormat vertexFormat);

} // namespace MTK

// Implementations

_NS_INLINE MTK::MeshBufferAllocator* MTK::MeshBufferAllocator::alloc()
{
    return NS::Object::alloc<MTK::MeshBufferAllocator>(_MTK_PRIVATE_CLS(MTKMeshBufferAllocator));
}

_NS_INLINE MTK::MeshBufferAllocator* MTK::MeshBufferAllocator::init(MTL::Device* pDevice)
{
    return Object::sendMessage<MTK::MeshBufferAllocator*>(this, _MTK_PRIVATE_SEL(initWithDevice_), pDevice);
}

_NS_INLINE MTL::Device* MTK::MeshBufferAllocator::device() const
{
    return Object::sendMessage<MTL::Device*>(this, _MTK_PRIVATE_SEL(device));
}

_NS_INLINE NS::UInteger MTK::MeshBuffer::length() const
{
    return Object::sendMessage<NS::UInteger>(this, _MTK_PRIVATE_SEL(length));
}

_NS_INLINE MTK::MeshBufferAllocator* MTK::MeshBuffer::allocator() const
{
    return Object::sendMessage<MTK::MeshBufferAllocator*>(this, _MTK_PRIVATE_SEL(allocator));
}

_NS_INLINE MDL::MeshBufferZone* MTK::MeshBuffer::zone() const
{
    return Object::sendMessage<MDL::MeshBufferZone*>(this, _MTK_PRIVATE_SEL(zone));
}

_NS_INLINE MTL::Buffer* MTK::MeshBuffer::buffer() const
{
    return Object::sendMessage<MTL::Buffer*>(this, _MTK_PRIVATE_SEL(buffer));
}

_NS_INLINE NS::UInteger MTK::MeshBuffer::offset() const
{
    return Object::sendMessage<NS::UInteger>(this, _MTK_PRIVATE_SEL(offset));
}

_NS_INLINE MDL::MeshBufferType MTK::MeshBuffer::type() const
{
    return Object::sendMessage<MDL::MeshBufferType>(this, _MTK_PRIVATE_SEL(type));
}

_NS_INLINE MTL::PrimitiveType MTK::Submesh::primitiveType() const
{
    return Object::sendMessage<MTL::PrimitiveType>(this, _MTK_PRIVATE_SEL(primitiveType));
}

_NS_INLINE MTL::IndexType MTK::Submesh::indexType() const
{
    return Object::sendMessage<MTL::IndexType>(this, _MTK_PRIVATE_SEL(indexType));
}

_NS_INLINE MTK::MeshBuffer* MTK::Submesh::indexBuffer() const
{
    return Object::sendMessage<MTK::MeshBuffer*>(this, _MTK_PRIVATE_SEL(indexBuffer));
}

_NS_INLINE NS::UInteger MTK::Submesh::indexCount() const
{
    return Object::sendMessage<NS::UInteger>(this, _MTK_PRIVATE_SEL(indexCount));
}

_NS_INLINE MTK::Mesh* MTK::Submesh::mesh() const
{
    return Object::sendMessage<MTK::Mesh*>(this, _MTK_PRIVATE_SEL(mesh));
}

_NS_INLINE NS::String* MTK::Submesh::name() const
{
    return Object::sendMessage<NS::String*>(this, _MTK_PRIVATE_SEL(name));
}

_NS_INLINE void MTK::Submesh::setName(const NS::String* pName)
{
    Object::sendMessage<void>(this, _MTK_PRIVATE_SEL(setName_), pName);
}

_NS_INLINE MTK::Mesh* MTK::Mesh::alloc()
{
    return NS::Object::alloc<MTK::Mesh>(_MTK_PRIVATE_CLS(MTKMesh));
}

_NS_INLINE MTK::Mesh* MTK::Mesh::init(MDL::Mesh* pMesh, MTL::Device* pDevice, NS::Error** ppError)
{
    return Object::sendMessage<MTK::Mesh*>(this, _MTK_PRIVATE_SEL(initWithMesh_device_error_), pMesh, pDevice, ppError);
}

_NS_INLINE NS::Array* MTK::Mesh::newMeshes(MDL::Asset* pAsset, MTL::Device* pDevice, NS::Array** ppSourceMeshes, NS::Error** ppError)
{
    return Object::sendMessage<NS::Array*>(_MTK_PRIVATE_CLS(MTKMesh), _MTK_PRIVATE_SEL(newMeshesFromAsset_device_sourceMeshes_error_), pAsset, pDevice, ppSourceMeshes, ppError);
}

_NS_INLINE NS::Array* MTK::Mesh::vertexBuffers() const
{
    return Object::sendMessage<NS::Array*>(this, _MTK_PRIVATE_SEL(vertexBuffers));
}

_NS_INLINE MDL::VertexDescriptor* MTK::Mesh::vertexDescriptor() const
{
    return Object::sendMessage<MDL::VertexDescriptor*>(this, _MTK_PRIVATE_SEL(vertexDescriptor));
}

_NS_INLINE NS::Array* MTK::Mesh::submeshes() const
{
    return Object::sendMessage<NS::Array*>(this, _MTK_PRIVATE_SEL(submeshes));
}

_NS_INLINE NS::UInteger MTK::Mesh::vertexCount() const
{
    return Object::sendMessage<NS::UInteger>(this, _MTK_PRIVATE_SEL(vertexCount));
}

_NS_INLINE NS::String* MTK::Mesh::name() const
{
    return Object::sendMessage<NS::String*>(this, _MTK_PRIVATE_SEL(name));
}

_NS_INLINE void MTK::Mesh::setName(const NS::String* pName)
{
    Object::sendMessage<void>(this, _MTK_PRIVATE_SEL(setName_), pName);
}

// Function Implementations (using Objective-C runtime for simplicity and consistency with metal-cpp)

extern "C" {
MTL::VertexDescriptor* MTKMetalVertexDescriptorFromModelIO(MDL::VertexDescriptor* modelIODescriptor);
MTL::VertexDescriptor* MTKMetalVertexDescriptorFromModelIOWithError(MDL::VertexDescriptor* modelIODescriptor, NS::Error** error);
MDL::VertexDescriptor* MTKModelIOVertexDescriptorFromMetal(MTL::VertexDescriptor* metalDescriptor);
MDL::VertexDescriptor* MTKModelIOVertexDescriptorFromMetalWithError(MTL::VertexDescriptor* metalDescriptor, NS::Error** error);
MTL::VertexFormat      MTKMetalVertexFormatFromModelIO(MDL::VertexFormat vertexFormat);
MDL::VertexFormat      MTKModelIOVertexFormatFromMetal(MTL::VertexFormat vertexFormat);
}

_NS_INLINE MTL::VertexDescriptor* MTK::MetalVertexDescriptorFromModelIO(MDL::VertexDescriptor* pModelDescriptor)
{
    return MTKMetalVertexDescriptorFromModelIO(pModelDescriptor);
}

_NS_INLINE MTL::VertexDescriptor* MTK::MetalVertexDescriptorFromModelIOWithError(MDL::VertexDescriptor* pModelDescriptor, NS::Error** ppError)
{
    return MTKMetalVertexDescriptorFromModelIOWithError(pModelDescriptor, ppError);
}

_NS_INLINE MDL::VertexDescriptor* MTK::ModelIOVertexDescriptorFromMetal(MTL::VertexDescriptor* pMetalDescriptor)
{
    return MTKModelIOVertexDescriptorFromMetal(pMetalDescriptor);
}

_NS_INLINE MDL::VertexDescriptor* MTK::ModelIOVertexDescriptorFromMetalWithError(MTL::VertexDescriptor* pMetalDescriptor, NS::Error** ppError)
{
    return MTKModelIOVertexDescriptorFromMetalWithError(pMetalDescriptor, ppError);
}

_NS_INLINE MTL::VertexFormat MTK::MetalVertexFormatFromModelIO(MDL::VertexFormat vertexFormat)
{
    return MTKMetalVertexFormatFromModelIO(vertexFormat);
}

_NS_INLINE MDL::VertexFormat MTK::ModelIOVertexFormatFromMetal(MTL::VertexFormat vertexFormat)
{
    return MTKModelIOVertexFormatFromMetal(vertexFormat);
}
