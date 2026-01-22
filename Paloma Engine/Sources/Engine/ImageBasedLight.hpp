//
//  ImageBasedLight.hpp
//  Paloma Engine
//
//  Created by Artem on 19.01.2026.
//

#pragma once

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>
#include <functional>
#include <simd/simd.h>
#include <mutex>
#include <string>

class ImageBasedLight {
public:
    NS::SharedPtr<MTL::Texture> diffuseCubeTexture;
    NS::SharedPtr<MTL::Texture> specularCubeTexture;
    int specularMipLevelCount;
    NS::SharedPtr<MTL::Texture> scaleAndBiasLookupTexture;
    NS::SharedPtr<MTL::SharedEvent> readyEvent;
    
    simd::float3x3 rotation = matrix_identity_float3x3;
    float intensity = 1.0f;
    
    ImageBasedLight(NS::SharedPtr<MTL::Texture> diffuseCubeTexture,
                    NS::SharedPtr<MTL::Texture> specularCubeTexture, int specularMipLevelCount,
                    NS::SharedPtr<MTL::Texture> scaleAndBiasLookupTexture,
                    NS::SharedPtr<MTL::SharedEvent> readyEvent);
    
    static void generateImageBasedLight(
                                        const std::string &url, MTL::Device *pDevice,
                                        std::function<void(ImageBasedLight *, NS::Error *)> completion);
};

class ImageBasedLightGenerator {
public:
    NS::SharedPtr<MTL::Device> _pDevice;
    NS::SharedPtr<MTL::CommandQueue> _pCommandQueue;
    
    NS::SharedPtr<MTL::ComputePipelineState> _pEquirectToCubePSO;
    NS::SharedPtr<MTL::ComputePipelineState> _pLookupTablePSO;
    NS::SharedPtr<MTL::ComputePipelineState> _pPrefilteringPSO;
    
public:
    ImageBasedLightGenerator(MTL::Device *pDevice,
                             MTL::CommandQueue *pCommandQueue);
    
    static ImageBasedLightGenerator *Default(MTL::Device *pDevice);
    
    ImageBasedLight *makeLight(const std::string &path);
};
