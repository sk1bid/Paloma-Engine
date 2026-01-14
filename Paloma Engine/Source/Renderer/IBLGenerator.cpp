//
//  IBLGenerator.cpp
//  Paloma Engine
//
//  Created by Artem on 12.01.2026.
//

#include "IBLGenerator.hpp"

namespace Paloma{

MTL::Texture* IBLGenerator::createCubemap(MTL::Device* device, uint32_t size, bool withMips) {
    auto desc = MTL::TextureDescriptor::alloc()->init();
    desc->setTextureType(MTL::TextureTypeCube);
    desc->setPixelFormat(MTL::PixelFormatRGBA16Float);
    desc->setWidth(size);
    desc->setHeight(size);
    desc->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite);
    desc->setStorageMode(MTL::StorageModePrivate);
    
    if (withMips) {
        uint32_t mipCount = (uint32_t)floor(log2((float)size)) + 1;
        desc->setMipmapLevelCount(mipCount);
    }
    
    auto texture = device->newTexture(desc);
    desc->release();
    return texture;
}

IBLResource IBLGenerator::generate(
    MTL::Device* device,
    MTL4::CommandQueue* queue,
    MTL4::CommandAllocator* allocator,
    MTL::Texture* equirectTexture,
    uint32_t cubemapSize
) {
    IBLResource result = {};
    
    result.environmentCubemap = createCubemap(device, cubemapSize, true);
    result.prefilteredMap = createCubemap(device, cubemapSize, true);
    result.brdfLut = createBRDFLut(device, 512);
    
    auto library = device->newDefaultLibrary();
    NS::Error* error = nullptr;
    
    auto cubeFromEquirectFunc = library->newFunction(
        NS::String::string("CubeFromEquirectangular", NS::UTF8StringEncoding)
    );
    auto cubeFromEquirectPSO = device->newComputePipelineState(cubeFromEquirectFunc, &error);
    
    auto prefilterFunc = library->newFunction(
        NS::String::string("PrefilterEnvironmentMap", NS::UTF8StringEncoding)
    );
    auto prefilterPSO = device->newComputePipelineState(prefilterFunc, &error);
    
    auto argTableDesc = MTL4::ArgumentTableDescriptor::alloc()->init();
    argTableDesc->setMaxBufferBindCount(4);
    argTableDesc->setMaxTextureBindCount(4);
    auto argTable = device->newArgumentTable(argTableDesc, nullptr);
    argTableDesc->release();
    
    auto paramsBuffer = device->newBuffer(sizeof(float), MTL::ResourceStorageModeShared);
    
    auto residencyDesc = MTL::ResidencySetDescriptor::alloc()->init();
    auto residencySet = device->newResidencySet(residencyDesc, nullptr);
    residencyDesc->release();
    
    residencySet->addAllocation(equirectTexture);
    residencySet->addAllocation(result.environmentCubemap);
    residencySet->addAllocation(result.prefilteredMap);
    residencySet->addAllocation(result.brdfLut);
    residencySet->addAllocation(paramsBuffer);
    residencySet->commit();
    
    queue->addResidencySet(residencySet);
    
    auto cmdBuffer = device->newCommandBuffer();
    allocator->reset();
    cmdBuffer->beginCommandBuffer(allocator);
    cmdBuffer->useResidencySet(residencySet);
    
    auto encoder = cmdBuffer->computeCommandEncoder();
    
    argTable->setTexture(equirectTexture->gpuResourceID(), 0);
    argTable->setTexture(result.environmentCubemap->gpuResourceID(), 1);
    
    encoder->setArgumentTable(argTable);
    encoder->setComputePipelineState(cubeFromEquirectPSO);
    
    MTL::Size gridSize = MTL::Size::Make(cubemapSize, cubemapSize, 6);
    MTL::Size threadgroupSize = MTL::Size::Make(8, 8, 1);
    encoder->dispatchThreads(gridSize, threadgroupSize);
    
    encoder->generateMipmaps(result.environmentCubemap);
    

    encoder->copyFromTexture(
        result.environmentCubemap, 0, 0,
        result.prefilteredMap, 0, 0,
        6, 1
    );
    
    encoder->setComputePipelineState(prefilterPSO);
    NS::UInteger mipCount = result.prefilteredMap->mipmapLevelCount();
    for (uint32_t mip = 1; mip < mipCount; ++mip) {
        float roughness = (float)mip / (float)(mipCount - 1);
        uint32_t mipSize = cubemapSize >> mip;
        if (mipSize < 1) mipSize = 1;
        
        memcpy(paramsBuffer->contents(), &roughness, sizeof(float));
        
        MTL::Texture* mipView = result.prefilteredMap->newTextureView(
            MTL::PixelFormatRGBA16Float,
            MTL::TextureTypeCube,
            NS::Range::Make(mip, 1),
            NS::Range::Make(0, 6)
        );
        
        argTable->setAddress(paramsBuffer->gpuAddress(), 0);
        argTable->setTexture(result.environmentCubemap->gpuResourceID(), 0);
        argTable->setTexture(mipView->gpuResourceID(), 1);
        
        encoder->setArgumentTable(argTable);
        
        MTL::Size mipGridSize = MTL::Size::Make(mipSize, mipSize, 6);
        encoder->dispatchThreads(mipGridSize, threadgroupSize);
        
        mipView->release();
    }
    encoder->endEncoding();
    cmdBuffer->endCommandBuffer();
    
    result.readyEvent = device->newEvent();
    
    const MTL4::CommandBuffer* buffers[] = {cmdBuffer};
    queue->commit(buffers, 1);
    queue->signalEvent(result.readyEvent, 1);
    
    // Cleanup
    cubeFromEquirectFunc->release();
    cubeFromEquirectPSO->release();
    prefilterFunc->release();
    prefilterPSO->release();
    library->release();
    cmdBuffer->release();
    argTable->release();
    paramsBuffer->release();
    
    return result;
}

MTL::Texture* IBLGenerator::createBRDFLut(MTL::Device* device, uint32_t size) {
    auto desc = MTL::TextureDescriptor::alloc()->init();
    desc->setTextureType(MTL::TextureType2D);
    desc->setPixelFormat(MTL::PixelFormatRG16Float);
    desc->setWidth(size);
    desc->setHeight(size);
    desc->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite);
    desc->setStorageMode(MTL::StorageModePrivate);
    
    auto texture = device->newTexture(desc);
    desc->release();
    return texture;
}
}
