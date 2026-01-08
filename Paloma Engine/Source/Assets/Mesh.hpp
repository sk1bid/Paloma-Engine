//
//  Mesh.hpp
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#pragma once

#include <Metal/Metal.hpp>
#include <vector>

namespace Paloma {

// -- Submesh - part of Mesh with one material --
struct Submesh {
    uint32_t indexStart; /// offset in index buffer
    uint32_t indexCount; /// Count of indices
    MTL::IndexType indexType;
};

// -- Mesh - GPU mesh with buffers and submeshes --
class Mesh {
public:
    Mesh(MTL::Buffer* vertexBuffer, MTL::Buffer* indexBuffer, std::vector<Submesh> submeshes) : _vertexBuffer(vertexBuffer), _indexBuffer(indexBuffer), _submeshes(std::move(submeshes)) {}
    
    ~Mesh() {
        if (_vertexBuffer){
            _vertexBuffer->release();
            _vertexBuffer = nullptr;
        }
        
        if (_indexBuffer) {
            _indexBuffer->release();
            _indexBuffer = nullptr;
        }
    }
    
    /// Cancel copying
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    
    /// Allow moving
    Mesh(Mesh&&) = default;
    Mesh& operator=(Mesh&&) = default;
    
    // -- Getters for MTL4::ArgumentTable --
    
    MTL::GPUAddress vertexAddress() const {
        return _vertexBuffer->gpuAddress();
    }
    
    MTL::GPUAddress indexAdress() const {
        return _indexBuffer->gpuAddress();
    }
    
    // -- Access to submeshes --
    
    const std::vector<Submesh>& submeshes() const {return _submeshes;}
    size_t submeshCount() const {return _submeshes.size();}
    
    // -- Access to buffers (for ResidencySet) --
    
    MTL::Buffer* vertexBuffer() const {return _vertexBuffer;}
    MTL::Buffer* indexBuffer() const {return _indexBuffer;}

    
private:
    MTL::Buffer* _indexBuffer;
    MTL::Buffer* _vertexBuffer;
    std::vector<Submesh> _submeshes;
};
}
