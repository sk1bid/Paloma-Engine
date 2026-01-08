//
//  PipelineCache.hpp
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#pragma once

#include <Metal/Metal.hpp>
#include <string>
#include <unordered_map>

namespace Paloma {

// -- pipeline descriptor for caching
struct PipelineDesc {
  std::string vertexFunction = "vertexMain";
  std::string fragmentFunction = "fragmentMain";
  MTL::PixelFormat colorFormat = MTL::PixelFormatBGRA8Unorm_sRGB;
  MTL::PixelFormat depthFormat = MTL::PixelFormatDepth32Float;

  // hash for caching
  size_t hash() const;
};

// -- cache compiled pipeline states --
/// we can use prepared pipeline without recompile

class PipelineCache {
public:
  void init(MTL::Device *device, MTL4::Compiler *compiler);
  void shutdown();

  /// Get or Create pipeline
  MTL::RenderPipelineState *getOrCreate(const PipelineDesc &desc);

  /// Get default pipeline (for quick start)
  MTL::RenderPipelineState *getDefault();

private:
  MTL::Device *_device = nullptr;
  MTL4::Compiler *_compiler = nullptr;
  MTL::Library *_library = nullptr;

  std::unordered_map<size_t, MTL::RenderPipelineState *> _cache;
  MTL::RenderPipelineState *_defaultPipeline = nullptr;
};
} // namespace Paloma
