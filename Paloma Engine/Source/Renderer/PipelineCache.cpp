//
//  PipelineCache.cpp
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#include "PipelineCache.hpp"
#include <functional>
#include "ShaderTypes.h"

namespace Paloma {

size_t PipelineDesc::hash() const {
    size_t h = 0;
    auto combine = [&h](size_t value) {
        h ^= value + 0x9e3779b9 + (h << 6) + (h >> 2);
    };
    combine(std::hash<std::string>{}(vertexFunction));
    combine(std::hash<std::string>{}(fragmentFunction));
    combine(std::hash<int>{}(static_cast<int>(colorFormat)));
    combine(std::hash<int>{}(static_cast<int>(depthFormat)));
    combine(std::hash<bool>{}(hasVertexInput));
    return h;
}

void PipelineCache::init(MTL::Device *device, MTL4::Compiler *compiler) {
    _device = device;
    _compiler = compiler;
    
    // Load default library (from bundle)
    _library = _device->newDefaultLibrary();
    if (!_library) {
        NS::Error *error = nullptr;
        NS::String *path =
        NS::String::string("Shaders.metallib", NS::UTF8StringEncoding);
        _library = _device->newLibrary(path, &error);
        
        if (error) {
            printf("PipelineCache: Failed to load library: %s\n",
                   error->localizedDescription()->utf8String());
        }
        
    }
    
    if (_library) {
        printf("PipelineCache: Library loaded!\n");
    } else {
        printf("PipelineCache: Library is NULL!\n");
    }
}

void PipelineCache::shutdown() {
    for (auto &[hash, pso] : _cache) {
        if (pso)
            pso->release();
    }
    _cache.clear();
    
    if (_skyboxPipeline) {
        _skyboxPipeline->release();
        _skyboxPipeline = nullptr;
    }
    
    if (_defaultPipeline) {
        _defaultPipeline->release();
        _defaultPipeline = nullptr;
    }
    
    if (_library) {
        _library->release();
        _library = nullptr;
    }
}

MTL::RenderPipelineState *PipelineCache::getOrCreate(const PipelineDesc &desc) {
    size_t h = desc.hash();
    
    // Check cache
    auto it = _cache.find(h);
    if (it != _cache.end()) {
        return it->second;
    }
    
    // Create pipeline descriptor
    auto pipelineDesc = MTL4::RenderPipelineDescriptor::alloc()->init();
    
    // Vertex function
    auto vertexFuncDesc = MTL4::LibraryFunctionDescriptor::alloc()->init();
    vertexFuncDesc->setLibrary(_library);
    vertexFuncDesc->setName(
                            NS::String::string(desc.vertexFunction.c_str(), NS::UTF8StringEncoding));
    pipelineDesc->setVertexFunctionDescriptor(vertexFuncDesc);
    
    // Fragment function
    auto fragmentFuncDesc = MTL4::LibraryFunctionDescriptor::alloc()->init();
    fragmentFuncDesc->setLibrary(_library);
    fragmentFuncDesc->setName(NS::String::string(desc.fragmentFunction.c_str(),
                                                 NS::UTF8StringEncoding));
    
    pipelineDesc->setFragmentFunctionDescriptor(fragmentFuncDesc);
    auto vertexDesc = MTL::VertexDescriptor::alloc()->init();
    if (desc.hasVertexInput){
        
        //0: Position (float3 = 12 bytes)
        vertexDesc->attributes()->object(VertexAttributePosition)->setFormat(MTL::VertexFormatFloat3);
        vertexDesc->attributes()->object(VertexAttributePosition)->setOffset(0);
        vertexDesc->attributes()->object(VertexAttributePosition)->setBufferIndex(BufferIndexVertices);
        
        //1: Normal (float3 = 12 bytes)
        vertexDesc->attributes()->object(VertexAttributeNormal)->setFormat(MTL::VertexFormatFloat3);
        vertexDesc->attributes()->object(VertexAttributeNormal)->setOffset(12);  // После position
        vertexDesc->attributes()->object(VertexAttributeNormal)->setBufferIndex(BufferIndexVertices);
        
        //2: Texcoord (float2 = 8 bytes)
        vertexDesc->attributes()->object(VertexAttributeTexcoord)->setFormat(MTL::VertexFormatFloat2);
        vertexDesc->attributes()->object(VertexAttributeTexcoord)->setOffset(24); // После position + normal
        vertexDesc->attributes()->object(VertexAttributeTexcoord)->setBufferIndex(BufferIndexVertices);
        
        // Stride = sizeof(Vertex) = 12 + 12 + 8 = 32 bytes
        vertexDesc->layouts()->object(BufferIndexVertices)->setStride(32);
        vertexDesc->layouts()->object(BufferIndexVertices)->setStepFunction(MTL::VertexStepFunctionPerVertex);
        
        pipelineDesc->setVertexDescriptor(vertexDesc);
    }
    // Color attachment (Metal 4 uses colorAttachments array)
    auto colorAttachment = pipelineDesc->colorAttachments()->object(0);
    colorAttachment->setPixelFormat(desc.colorFormat);
    
    // Compile pipeline
    NS::Error *error = nullptr;
    auto pso = _compiler->newRenderPipelineState(pipelineDesc, nullptr, &error);
    
    // Cleanup descriptors
    vertexFuncDesc->release();
    fragmentFuncDesc->release();
    vertexDesc->release();
    pipelineDesc->release();
    
    if (!pso) {
        if (error) {
            printf("PipelineCache: Pipeline compilation failed: %s\n",
                   error->localizedDescription()->utf8String());
        } else {
            printf("PipelineCache: Pipeline is NULL but no error!\n");
        }
        return nullptr;
    }
    
    printf("PipelineCache: Pipeline created for %s/%s\n",
           desc.vertexFunction.c_str(), desc.fragmentFunction.c_str());
    
    _cache[h] = pso;
    return pso;
}

MTL::RenderPipelineState *PipelineCache::getDefault() {
    if (!_defaultPipeline) {
        PipelineDesc desc;
        desc.vertexFunction = "vertexMain";
        desc.fragmentFunction = "fragmentMain";
        _defaultPipeline = getOrCreate(desc);
    }
    return _defaultPipeline;
}

MTL::RenderPipelineState* PipelineCache::getSkybox() {
    if (!_skyboxPipeline) {
        PipelineDesc desc;
        desc.vertexFunction = "skyboxVertex";
        desc.fragmentFunction = "skyboxFragment";
        desc.hasVertexInput = false;
        _skyboxPipeline = getOrCreate(desc);
    }
    return _skyboxPipeline;
}

} // namespace Paloma
