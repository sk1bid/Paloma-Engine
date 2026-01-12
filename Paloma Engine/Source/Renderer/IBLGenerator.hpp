//
//  IBLGenerator.hpp
//  Paloma Engine
//
//  Created by Artem on 12.01.2026.
//

#pragma once
#include <Metal/Metal.hpp>
#include <string>

namespace Paloma {

struct IBLResource {
    MTL::Texture* irradianceMap = nullptr; // diffuse
    MTL::Texture* prefilteredMap = nullptr; // Specular with mipmaps
    MTL::Texture* brdfLut = nullptr;
    
    void release() {
        if (irradianceMap){
            irradianceMap->release();
        }
        if (prefilteredMap){
            prefilteredMap->release();
        }
        if (brdfLut){
            brdfLut->release();
        }
    }
};

class IBLGenerator {
public:
    static IBLResource generate(MTL::Device* device,
                                MTL::CommandQueue* queue,
                                const std::string& hdrPath);
    
private:
    static MTL::Texture* createSpecularMap(MTL::Device* device, uint32_t size);
    static MTL::Texture* createIrradianceMap(MTL::Device* device, uint32_t size);
    static MTL::Texture* createBrdfLut(MTL::Device* device, uint32_t size);
};
}
