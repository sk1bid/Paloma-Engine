//
//  PipelineCache.cpp
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#include "PipelineCache.hpp"
#include <functional>

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
  return h;
}

void PipelineCache::init(MTL::Device *device, MTL4::Compiler *compiler) {
  _device = device;
  _compiler = compiler;

  // Load default library (compiled shaders from bundle)
  _library = _device->newDefaultLibrary();

  // Debug: check if library loaded
  if (!_library) {
    // Library failed to load - shaders not compiled into bundle
  }
}

void PipelineCache::shutdown() {
  for (auto &[hash, pso] : _cache) {
    if (pso)
      pso->release();
  }
  _cache.clear();

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
  // Check requirements
  if (!_library || !_compiler) {
    return nullptr;
  }

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

  // Color attachment
  auto colorAttachment = pipelineDesc->colorAttachments()->object(0);
  colorAttachment->setPixelFormat(desc.colorFormat);

  // Compile pipeline
  NS::Error *error = nullptr;
  auto pso = _compiler->newRenderPipelineState(pipelineDesc, nullptr, &error);

  // Cleanup descriptors
  vertexFuncDesc->release();
  fragmentFuncDesc->release();
  pipelineDesc->release();

  if (!pso) {
    // Compilation failed
    if (error) {
      // Error details available but can't log from C++
    }
    return nullptr;
  }

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

} // namespace Paloma
