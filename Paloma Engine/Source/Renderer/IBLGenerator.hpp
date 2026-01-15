//
//  IBLGenerator.hpp
//  Paloma Engine
//
//  Created by Artem on 12.01.2026.
//

#pragma once
#include <Metal/Metal.hpp>
#include "ShaderTypes.h"
#include <string>

namespace Paloma {

struct IBLResource {
    MTL::Texture* environmentCubemap = nullptr;
    MTL::Texture* prefilteredMap = nullptr;
    MTL::Texture* brdfLut = nullptr;
    MTL::Event* readyEvent = nullptr;
    MTL::Buffer* shBuffer = nullptr;
};

class IBLGenerator {
public:
    static IBLResource generate(
                                MTL::Device* device,
                                MTL4::CommandQueue* queue,
                                MTL4::CommandAllocator* allocator,
                                MTL::Texture* equirectTexture,
                                uint32_t cubemapSize = 512
                                );
    
private:
    static MTL::Texture* createCubemap(MTL::Device* device, uint32_t size, bool withMips);
    static MTL::Texture* createBRDFLut(MTL::Device* device, uint32_t size);
};

}
