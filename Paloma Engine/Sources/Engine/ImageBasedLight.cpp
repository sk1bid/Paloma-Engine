//
//  ImageBasedLight.cpp
//  Paloma Engine
//
//  Created by Artem on 19.01.2026.
//

#include "ImageBasedLight.hpp"

ImageBasedLight::ImageBasedLight(NS::SharedPtr<MTL::Texture> diffuseCubeTexture,
                                 NS::SharedPtr<MTL::Texture> specularCubeTexture, int specularMipLevelCount,
                                 NS::SharedPtr<MTL::Texture> scaleAndBiasLookupTexture,
                                 NS::SharedPtr<MTL::Event> readyEvent) : diffuseCubeTexture(diffuseCubeTexture), specularCubeTexture(specularCubeTexture), specularMipLevelCount(specularMipLevelCount),scaleAndBiasLookupTexture(scaleAndBiasLookupTexture), readyEvent(readyEvent)
{}

void ImageBasedLight::generateImageBasedLight(const std::string &url, MTL::Device *pDevice, std::function<void (ImageBasedLight *, NS::Error *)> completion)
{
//    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
//        
//        
//        ImageBasedLightGenerator* pGenerator = ImageBasedLightGenerator::Default(pDevice);
//        
//        ImageBasedLight* pLight = pGenerator->makeLight(url);
//        
//        if (pLight) {
//            completion(pLight, nullptr);
//        } else {
//            completion(nullptr, nullptr);
//        }
//    });
//    
    ImageBasedLightGenerator* pGenerator = ImageBasedLightGenerator::Default(pDevice);
      
      // makeLight делает тяжелую работу, но это безопасно для Main Thread при инициализации
      ImageBasedLight* pLight = pGenerator->makeLight(url);
      
      if (pLight) {
          // Вызываем callback сразу
          completion(pLight, nullptr);
      } else {
          NS::Error* pError = NS::Error::error(NS::String::string("Failed to load IBL", NS::UTF8StringEncoding), 0, nullptr);
          completion(nullptr, pError);
      }
}

ImageBasedLightGenerator* ImageBasedLightGenerator::Default(MTL::Device* pDevice) {
    static ImageBasedLightGenerator* s_pGenerator = nullptr;
    static std::once_flag s_once;
    
    std::call_once(s_once, [pDevice](){
        // Создаем отдельную очередь для генерации, чтобы не тормозить кадры
        MTL::CommandQueue* pQueue = pDevice->newCommandQueue();
        s_pGenerator = new ImageBasedLightGenerator(pDevice, pQueue);
        pQueue->release();
    });
    
    return s_pGenerator;
}

ImageBasedLightGenerator::ImageBasedLightGenerator(MTL::Device* pDevice, MTL::CommandQueue* pCommandQueue)
: _pDevice(NS::RetainPtr(pDevice))
, _pCommandQueue(NS::RetainPtr(pCommandQueue))
{
    auto pLibrary = NS::TransferPtr(_pDevice->newDefaultLibrary());
    
    auto loadPSO = [&](const char* funcName) -> NS::SharedPtr<MTL::ComputePipelineState> {
        auto pFuncStr = NS::String::string(funcName, NS::UTF8StringEncoding);
        auto pFunc = NS::TransferPtr(pLibrary->newFunction(pFuncStr));
        NS::Error* pError = nullptr;
        return NS::TransferPtr(_pDevice->newComputePipelineState(pFunc.get(), &pError));
    };
    _pEquirectToCubePSO = loadPSO("CubeFromEquirectangular");
    _pLookupTablePSO    = loadPSO("F0OffsetAndBiasLUT");
    _pPrefilteringPSO   = loadPSO("PrefilterEnvironmentMap");
}

ImageBasedLight* ImageBasedLightGenerator::makeLight(const std::string &path) {
    using namespace MTL;
    // 1. Loading Base Texture
    auto textureLoader = NS::TransferPtr(MTK::TextureLoader::alloc()->init(_pDevice.get()));
    
    auto srgbKey = MTK::TextureLoaderOptionSRGB;
    auto mipKey = MTK::TextureLoaderOptionAllocateMipmaps;
    auto valFalse = NS::Number::number(false);
    NS::Object* keys[] = { (NS::Object*)srgbKey, (NS::Object*)mipKey };
    NS::Object* values[] = { (NS::Object*)valFalse, (NS::Object*)valFalse };
    
    // FIX: dictionary returns autoreleased object -> No TransferPtr
    NS::Dictionary* hdrOptions = NS::Dictionary::dictionary(values, keys, 2);
    
    NS::Error* error = nullptr;
    // FIX: fileURLWithPath returns autoreleased -> No TransferPtr
    NS::URL* url = NS::URL::fileURLWithPath(NS::String::string(path.c_str(), NS::UTF8StringEncoding));
    
    auto equirectTexture = NS::TransferPtr(textureLoader->newTexture(url, hdrOptions, &error));
    
    if (!equirectTexture) {
        printf("Error loading HDR: %s\n", error->localizedDescription()->utf8String());
        return nullptr;
    }
    
    // Параметры
    PixelFormat workingPixelFormat = PixelFormatRGBA16Float;
    NS::UInteger sourceHeight = equirectTexture->height();
    NS::UInteger sourceCubeSize = std::min((NS::UInteger)512, sourceHeight / 2);
    NS::UInteger specularCubeSize = sourceCubeSize;
    NS::UInteger diffuseCubeSize = 32;
    NS::UInteger lookupTableSize = 512;
    NS::UInteger specularSampleCount = 1024;
    NS::UInteger diffuseSampleCount = 2048;
    NS::UInteger lutSampleCount = 512;
    
    // 2. Texture Descriptors & Allocation
    // FIX: factory methods return autoreleased -> No TransferPtr
    auto sourceCubeDescriptor = TextureDescriptor::textureCubeDescriptor(workingPixelFormat, sourceCubeSize, true);
    sourceCubeDescriptor->setUsage(TextureUsageShaderRead | TextureUsageShaderWrite);
    auto sourceCubeTexture = NS::TransferPtr(_pDevice->newTexture(sourceCubeDescriptor));
    sourceCubeTexture->setLabel(NS::String::string("Environment Map (Cube)", NS::UTF8StringEncoding));
    
    auto specularCubeDescriptor = TextureDescriptor::textureCubeDescriptor(workingPixelFormat, specularCubeSize, true);
    specularCubeDescriptor->setUsage(TextureUsageShaderRead | TextureUsageShaderWrite);
    specularCubeDescriptor->setStorageMode(StorageModePrivate);
    auto specularCubeTexture = NS::TransferPtr(_pDevice->newTexture(specularCubeDescriptor));
    specularCubeTexture->setLabel(NS::String::string("Prefiltered Environment (GGX)", NS::UTF8StringEncoding));
    
    auto diffuseCubeDescriptor = TextureDescriptor::textureCubeDescriptor(workingPixelFormat, diffuseCubeSize, false);
    diffuseCubeDescriptor->setUsage(TextureUsageShaderRead | TextureUsageShaderWrite);
    diffuseCubeDescriptor->setStorageMode(StorageModePrivate);
    auto diffuseCubeTexture = NS::TransferPtr(_pDevice->newTexture(diffuseCubeDescriptor));
    diffuseCubeTexture->setLabel(NS::String::string("Prefiltered Environment (Lambertian)", NS::UTF8StringEncoding));
    
    auto lookupTextureDescriptor = TextureDescriptor::texture2DDescriptor(PixelFormatRGBA16Float, lookupTableSize, lookupTableSize, false);
    lookupTextureDescriptor->setUsage(TextureUsageShaderRead | TextureUsageShaderWrite);
    lookupTextureDescriptor->setStorageMode(StorageModePrivate);
    auto lookupTexture = NS::TransferPtr(_pDevice->newTexture(lookupTextureDescriptor));
    lookupTexture->setLabel(NS::String::string("DFG Lookup Table (GGX)", NS::UTF8StringEncoding));
    
    // 3. Pass 1: Equirect -> Cube
    // FIX: commandBuffer returns autoreleased -> No TransferPtr
    auto commandBuffer = _pCommandQueue->commandBuffer();
    // FIX: computeCommandEncoder returns autoreleased -> No TransferPtr
    auto computeCommandEncoder = commandBuffer->computeCommandEncoder();
    
    computeCommandEncoder->setComputePipelineState(_pEquirectToCubePSO.get());
    computeCommandEncoder->setTexture(equirectTexture.get(), 1);
    computeCommandEncoder->setTexture(sourceCubeTexture.get(), 0);
    computeCommandEncoder->dispatchThreads(MTL::Size::Make(sourceCubeSize, sourceCubeSize, 6),
                                           MTL::Size::Make(32, 32, 1));
    computeCommandEncoder->endEncoding();
    commandBuffer->commit();
    
    // 4. Mipmaps & All Other Computed Passes
    commandBuffer = _pCommandQueue->commandBuffer();
    
    auto mipmapCommandEncoder = commandBuffer->blitCommandEncoder();
    mipmapCommandEncoder->generateMipmaps(sourceCubeTexture.get());
    mipmapCommandEncoder->endEncoding();
    
    computeCommandEncoder = commandBuffer->computeCommandEncoder();
    
    struct PrefilteringParams {
        uint32_t distribution;
        uint32_t sampleCount;
        float roughness;
        float lodBias;
        float cubemapSize;
    };
    
    int mipLevelCount = 5;
    NS::UInteger levelSize = specularCubeSize;
    
    for (int mipLevel = 0; mipLevel < mipLevelCount; mipLevel++) {
        // newTextureView starts with 'new' -> Retained -> TransferPtr OK
        auto levelView = NS::TransferPtr(specularCubeTexture->newTextureView(workingPixelFormat, TextureTypeCube, NS::Range(mipLevel, 1), NS::Range(0, 6)));
        
        PrefilteringParams params;
        params.distribution = 1; // GGX
        params.sampleCount = (uint32_t)specularSampleCount;
        params.roughness = (float)mipLevel / (float)(mipLevelCount - 1);
        params.lodBias = 0.0f;
        params.cubemapSize = (float)sourceCubeSize;
        
        computeCommandEncoder->setComputePipelineState(_pPrefilteringPSO.get());
        computeCommandEncoder->setBytes(&params, sizeof(params), 0);
        computeCommandEncoder->setTexture(sourceCubeTexture.get(), 0);
        computeCommandEncoder->setTexture(levelView.get(), 1);
        computeCommandEncoder->dispatchThreads(MTL::Size::Make(levelSize, levelSize, 6),
                                               MTL::Size::Make(32, 32, 1));
        
        levelSize >>= 1;
    }
    
    // Diffuse Pass
    PrefilteringParams diffuseParams;
    diffuseParams.distribution = 0; // Lambert
    diffuseParams.sampleCount = (uint32_t)diffuseSampleCount;
    diffuseParams.roughness = 0.0f;
    diffuseParams.lodBias = 0.0f;
    diffuseParams.cubemapSize = (float)sourceCubeSize;
    
    computeCommandEncoder->setComputePipelineState(_pPrefilteringPSO.get());
    computeCommandEncoder->setBytes(&diffuseParams, sizeof(diffuseParams), 0);
    computeCommandEncoder->setTexture(sourceCubeTexture.get(), 0);
    computeCommandEncoder->setTexture(diffuseCubeTexture.get(), 1);
    computeCommandEncoder->dispatchThreads(MTL::Size::Make(diffuseCubeSize, diffuseCubeSize, 6),
                                           MTL::Size::Make(32, 32, 1));
    
    // LUT Pass
    uint32_t lutSamples = (uint32_t)lutSampleCount;
    computeCommandEncoder->setComputePipelineState(_pLookupTablePSO.get());
    computeCommandEncoder->setBytes(&lutSamples, sizeof(lutSamples), 0);
    computeCommandEncoder->setTexture(lookupTexture.get(), 0);
    computeCommandEncoder->dispatchThreads(MTL::Size::Make(lookupTableSize, lookupTableSize, 1),
                                           MTL::Size::Make(32, 32, 1));
    computeCommandEncoder->endEncoding();
    
    // Finalize
    auto readyEvent = NS::TransferPtr(_pDevice->newEvent());
    commandBuffer->encodeSignalEvent(readyEvent.get(), 1);
    commandBuffer->commit();
    
    return new ImageBasedLight(diffuseCubeTexture, specularCubeTexture, mipLevelCount, lookupTexture, readyEvent);
}
