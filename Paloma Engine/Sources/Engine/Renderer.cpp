//
//  Renderer.cpp
//  Paloma Engine
//
//  Created by Artem on 20.01.2026.
//
#include "Renderer.hpp"
#include "Entity.hpp"
#include "ShaderStructures.h"
#include <algorithm>
#include <cstdio>

extern "C" double CACurrentMediaTime();

struct DrawCall {
  std::shared_ptr<Mesh> mesh;
  const Submesh *submesh;
  Material *material;
  matrix_float4x4 modelTransform;
  simd_float3 modelViewPosition;
};

RendererInterface *CreateRenderer(MTL::Device *pDevice) {
  if (pDevice->supportsFamily(MTL::GPUFamilyMetal4)) {

    return new Metal4Renderer(pDevice);
  } else {

    exit(0);
  }
}

Metal4Renderer::Metal4Renderer(MTL::Device *pDevice)
    : _pDevice(NS::RetainPtr(pDevice)) {

  _colorPixelFormat = MTL::PixelFormatBGRA8Unorm_sRGB;
  _depthStencilPixelFormat = MTL::PixelFormatDepth32Float;

  _pCommandQueue = NS::TransferPtr(_pDevice->newMTL4CommandQueue());
  _pCommandBuffer = NS::TransferPtr(_pDevice->newCommandBuffer());

  for (int i = 0; i < kMaxFramesInFlight; i++) {
    _pCommandAllocators.push_back(
        NS::TransferPtr(_pDevice->newCommandAllocator()));
  }

  const uint32_t sampleCounts[] = {8, 4, 2, 1}; // for MSAA
  _rasterSampleCount = 1;                       // default - no MSAA

  for (uint count : sampleCounts) {
    if (_pDevice->supportsTextureSampleCount(count)) {
      _rasterSampleCount = count;
      break; // choose max samle count on current GPU
    }
  }

  _pFrameCompletionEvent = NS::TransferPtr(_pDevice->newSharedEvent());

  _pLibrary = NS::TransferPtr(_pDevice->newDefaultLibrary());

  // -- Create Compiler --
  auto pCompilerDesc =
      NS::TransferPtr(MTL4::CompilerDescriptor::alloc()->init());
  NS::Error *pError = nullptr;

  _pCompiler =
      NS::TransferPtr(_pDevice->newCompiler(pCompilerDesc.get(), &pError));
  if (!_pCompiler) {

    assert(false);
  }

  // -- Create Residency Set --
  auto pResidencyDesc =
      NS::TransferPtr(MTL::ResidencySetDescriptor::alloc()->init());

  pResidencyDesc->setInitialCapacity(64);

  _pResidencySet =
      NS::TransferPtr(_pDevice->newResidencySet(pResidencyDesc.get(), &pError));
  if (!_pResidencySet) {

    assert(false);
  }

  // -- Create Argument Tables --
  auto argumentDescriptor =
      NS::TransferPtr(MTL4::ArgumentTableDescriptor::alloc()->init());
  argumentDescriptor->setMaxBufferBindCount(
      std::max((uint32_t)VertexBufferCount, (uint32_t)FragmentBufferCount));
  argumentDescriptor->setMaxTextureBindCount(FragmentTextureCount);

  _pVertexArgumentTable = NS::TransferPtr(
      _pDevice->newArgumentTable(argumentDescriptor.get(), &pError));
  if (!_pVertexArgumentTable) {

    assert(false);
  }
  _pFragmentArgumentTable = NS::TransferPtr(
      _pDevice->newArgumentTable(argumentDescriptor.get(), &pError));
  if (!_pFragmentArgumentTable) {

    assert(false);
  }

  for (int i = 0; i < kMaxFramesInFlight; i++) {
    _pConstantBuffers[i] = new RingBuffer(64 * 1024, _pDevice.get());
  }

  _pMaterialsBuffer = new RingBuffer(64 * 1024, _pDevice.get());

  // -- Create Depth Stencil States --
  auto depthStencilDescriptor =
      NS::TransferPtr(MTL::DepthStencilDescriptor::alloc()->init());
  depthStencilDescriptor->setDepthWriteEnabled(true);
  depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLess);

  // -- Opaque and Mask (Example: rock and foliage) --
  _pDepthStencilStates[(uint32_t)AlphaMode::Opaque] = NS::TransferPtr(
      _pDevice->newDepthStencilState(depthStencilDescriptor.get()));
  _pDepthStencilStates[(uint32_t)AlphaMode::Mask] = NS::TransferPtr(
      _pDevice->newDepthStencilState(depthStencilDescriptor.get()));

  // -- Blend (Water, Glass..) --
  depthStencilDescriptor->setDepthWriteEnabled(false);
  _pDepthStencilStates[(uint32_t)AlphaMode::Blend] = NS::TransferPtr(
      _pDevice->newDepthStencilState(depthStencilDescriptor.get()));

  _pFrameCompletionEvent->setSignaledValue(_frameIndex);

  //    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT,
  //    0),
  //                   ^{
  //        makeResources();
  //    });
  makeResources();
}

void Metal4Renderer::makeResources() {
  auto *pBundle = NS::Bundle::mainBundle();

  // 1. Получаем пути
  auto *pScenePathNS = pBundle->pathForResource(
      NS::String::string("Island", NS::UTF8StringEncoding),
      NS::String::string("usdz", NS::UTF8StringEncoding));

  auto *pEnvPathNS = pBundle->pathForResource(
      NS::String::string("Beach", NS::UTF8StringEncoding),
      NS::String::string("hdr", NS::UTF8StringEncoding));

  std::string scenePath = pScenePathNS ? pScenePathNS->utf8String() : "";
  std::string envPathStr = pEnvPathNS ? pEnvPathNS->utf8String() : "";

  if (scenePath.empty() || envPathStr.empty()) {

    return;
  }

  // 2. Загружаем сцену

  _pScene.reset(Scene::load(scenePath, _pDevice.get()));
  if (!_pScene) {

    return;
  }

  // 3. Загружаем IBL

  ImageBasedLight::generateImageBasedLight(
      envPathStr, _pDevice.get(),
      [this](ImageBasedLight *pLight, NS::Error *pError) {
        if (pError) {

          return;
        }
        if (!_pScene) {

          return;
        }

        _pScene->pLightingEnvironment = pLight;
        if (_pScene->pLightingEnvironment) {
          _pScene->pLightingEnvironment->intensity = 3.0f;
        }

        _pScene->lights.push_back(Light());

        _pScene->rootEntity->visitHierarchy([this](Entity *pEntity) {
          auto pModelEntity = dynamic_cast<ModelEntity *>(pEntity);
          if (pModelEntity && pModelEntity->mesh) {
            // printf("[Debug] Processing Mesh: %s\n",
            // pModelEntity->mesh->name.c_str());
            auto &mesh = pModelEntity->mesh;

            for (auto &material : mesh->materials) {
              if (material.alphaMode == AlphaMode::Blend) {
                material.alphaMode = AlphaMode::Mask;
              }

              // printf("[Debug] Creating Pipeline State...\n");
              material.pRenderPipelineState =
                  makePipelineState(mesh.get(), &material);

              MaterialConstants constants;
              constants.opacityFactor = material.opacity.factor;
              constants.baseColorFactor = {material.baseColor.factor.x,
                                           material.baseColor.factor.y,
                                           material.baseColor.factor.z, 1.0f};
              constants.metallicFactor = material.metalness.factor;
              constants.roughnessFactor = material.roughness.factor;
              constants.emissiveFactor = material.emissive.factor;
              constants.alphaCutoff = material.alphaThreshold;
              constants.normalScale = material.normal.factor;
              constants.occlusionStrength = material.occlusion.factor;

              MaterialArguments args;
              args.constants = constants;

              auto getID = [](const NS::SharedPtr<MTL::Texture> &t) {
                return t ? t->gpuResourceID() : MTL::ResourceID{0};
              };

              args.baseColorTexture = getID(material.baseColor.pTexture);
              args.normalTexture = getID(material.normal.pTexture);
              args.emissiveTexture = getID(material.emissive.pTexture);
              args.roughnessTexture = getID(material.roughness.pTexture);
              args.metalnessTexture = getID(material.metalness.pTexture);
              args.occlusionTexture = getID(material.occlusion.pTexture);
              args.opacityTexture = getID(material.opacity.pTexture);

              if (_pMaterialsBuffer) {
                material.bufferView = _pMaterialsBuffer->copy(args);
              }
            }
          }
        });

        if (auto pCameraNode = _pScene->rootEntity->childNamed("Camera")) {
          matrix_float4x4 camWorld = pCameraNode->worldTransform();
          _camera.position = camWorld.columns[3].xyz;

          matrix_float3x3 rotMat;
          rotMat.columns[0] = camWorld.columns[0].xyz;
          rotMat.columns[1] = camWorld.columns[1].xyz;
          rotMat.columns[2] = camWorld.columns[2].xyz;
          _camera.orientation = simd_quaternion(rotMat);

          _camera.nearZ = 0.01f;
          _camera.farZ = 500.0f;
        } else {
        }

        makeSceneResourcesResident(_pScene.get());

        _hasPreparedResources = true;
      });
}

void Metal4Renderer::makeSceneResourcesResident(Scene *scene) {
  // 1. Сценарные ресурсы (текстуры и буферы из Scene)
  const auto &sceneResources = scene->getResources();
  if (!sceneResources.empty()) {
    std::vector<const MTL::Allocation *> allocations;
    allocations.reserve(sceneResources.size());
    for (const auto &res : sceneResources) {
      // В metal-cpp MTL::Resource не наследуется от MTL::Allocation на уровне
      // C++ типов, поэтому используем reinterpret_cast для передачи в ObjC
      // runtime
      allocations.push_back(
          reinterpret_cast<const MTL::Allocation *>(res.get()));
    }
    _pResidencySet->addAllocations(allocations.data(), allocations.size());
  }

  // 2. Кольцевые константные буферы
  // Т.к. _pConstantBuffers — это массив указателей RingBuffer*, используем
  // оператор ->
  std::vector<const MTL::Allocation *> constantAllocations;
  constantAllocations.reserve(kMaxFramesInFlight);
  for (int i = 0; i < kMaxFramesInFlight; ++i) {
    if (_pConstantBuffers[i]) {
      constantAllocations.push_back(reinterpret_cast<const MTL::Allocation *>(
          _pConstantBuffers[i]->getBuffer()));
    }
  }
  _pResidencySet->addAllocations(constantAllocations.data(),
                                 constantAllocations.size());

  // 3. Буфер материалов (одиночное распределение)
  if (_pMaterialsBuffer) {
    _pResidencySet->addAllocation(reinterpret_cast<const MTL::Allocation *>(
        _pMaterialsBuffer->getBuffer()));
  }

  // 4. Окружение IBL (Image Based Light)
  // Используем прямой доступ к публичным SharedPtr полям класса ImageBasedLight
  if (auto *light = scene->getLightingEnvironment()) {
    const MTL::Allocation *iblAllocations[3] = {
        reinterpret_cast<const MTL::Allocation *>(
            light->diffuseCubeTexture.get()),
        reinterpret_cast<const MTL::Allocation *>(
            light->specularCubeTexture.get()),
        reinterpret_cast<const MTL::Allocation *>(
            light->scaleAndBiasLookupTexture.get())};
    _pResidencySet->addAllocations(iblAllocations, 3);
  }

  // 5. Регистрация в очереди и фиксация набора (Metal 3.2+ / GPUFamilyMetal4)
  // Метод addResidencySet ожидает сырой указатель, используем .get()
  _pCommandQueue->addResidencySet(_pResidencySet.get());
  _pResidencySet->commit();

  _hasPreparedResources = true;
}

NS::SharedPtr<MTL::RenderPipelineState>
Metal4Renderer::makePipelineState(Mesh *pMesh, Material *pMaterial) {
  bool hasNormals = pMesh->vertexDescriptor->attributeNamed(
                        MDL::VertexAttributeNormal) != nil;
  bool hasTangents = pMesh->vertexDescriptor->attributeNamed(
                         MDL::VertexAttributeTangent) != nil;
  bool hasTexCoords0 = pMesh->vertexDescriptor->attributeNamed(
                           MDL::VertexAttributeTextureCoordinate) != nil;

  int texCoordCount = 0;
  NS::Array *attributes = pMesh->vertexDescriptor->attributes();
  for (NS::UInteger i = 0; i < attributes->count(); ++i) {
    MDL::VertexAttribute *attr = (MDL::VertexAttribute *)attributes->object(i);
    if (attr->name()->isEqualToString(MDL::VertexAttributeTextureCoordinate)) {
      texCoordCount++;
    }
  }
  bool hasTexCoords1 = texCoordCount > 1;

  bool hasVertexColors =
      pMesh->vertexDescriptor->attributeNamed(MDL::VertexAttributeColor) != nil;
  bool hasBaseColorTexture = (pMaterial->baseColor.pTexture.get() != nullptr);
  bool hasEmissiveTexture = (pMaterial->emissive.pTexture.get() != nullptr);
  bool hasNormalTexture = (pMaterial->normal.pTexture.get() != nullptr);
  bool hasMetalnessTexture = (pMaterial->metalness.pTexture.get() != nullptr);
  bool hasRoughnessTexture = (pMaterial->roughness.pTexture.get() != nullptr);
  bool hasOcclusionTexture = (pMaterial->occlusion.pTexture.get() != nullptr);
  bool hasOpacityTexture = (pMaterial->opacity.pTexture.get() != nullptr);

  int baseColorUVSet = pMaterial->baseColor.mappingChannel;
  int emissiveUVSet = pMaterial->emissive.mappingChannel;
  int normalUVSet = pMaterial->normal.mappingChannel;
  int metalnessUVSet = pMaterial->metalness.mappingChannel;
  int roughnessUVSet = pMaterial->roughness.mappingChannel;
  int occlusionUVSet = pMaterial->occlusion.mappingChannel;
  int opacityUVSet = pMaterial->opacity.mappingChannel;

  bool useIBL = (_pScene->pLightingEnvironment != nullptr);

  uint32_t alphaMode = (uint32_t)pMaterial->alphaMode;
  NS::SharedPtr<MTL::FunctionConstantValues> functionConstants;
  functionConstants =
      NS::TransferPtr(MTL::FunctionConstantValues::alloc()->init());

  auto setBool = [&](bool val, const char *name) {
    functionConstants->setConstantValue(
        &val, MTL::DataTypeBool,
        NS::String::string(name, NS::UTF8StringEncoding));
  };
  auto setInt = [&](int val, const char *name) {
    functionConstants->setConstantValue(
        &val, MTL::DataTypeInt,
        NS::String::string(name, NS::UTF8StringEncoding));
  };
  auto setUInt = [&](uint32_t val, const char *name) {
    functionConstants->setConstantValue(
        &val, MTL::DataTypeUInt,
        NS::String::string(name, NS::UTF8StringEncoding));
  };
  setBool(hasNormals, "hasNormals");
  setBool(hasTangents, "hasTangents");
  setBool(hasTexCoords0, "hasTexCoords0");
  setBool(hasTexCoords1, "hasTexCoords1");
  setBool(hasVertexColors, "hasColors");
  setBool(hasBaseColorTexture, "hasBaseColorMap");
  setInt(baseColorUVSet, "baseColorUVSet");
  setBool(hasEmissiveTexture, "hasEmissiveMap");
  setInt(emissiveUVSet, "emissiveUVSet");
  setBool(hasNormalTexture, "hasNormalMap");
  setInt(normalUVSet, "normalUVSet");
  setBool(hasMetalnessTexture, "hasMetalnessMap");
  setInt(metalnessUVSet, "metalnessUVSet");
  setBool(hasRoughnessTexture, "hasRoughnessMap");
  setInt(roughnessUVSet, "roughnessUVSet");
  setBool(hasOcclusionTexture, "hasOcclusionMap");
  setInt(occlusionUVSet, "occlusionUVSet");
  setBool(hasOpacityTexture, "hasOpacityMap");
  setInt(opacityUVSet, "opacityUVSet");
  setBool(useIBL, "useIBL");
  setUInt(alphaMode, "alphaMode");

  auto vertexFunction =
      NS::TransferPtr(MTL4::SpecializedFunctionDescriptor::alloc()->init());
  vertexFunction->setSpecializedName(
      NS::String::string("pbr_vertex", NS::UTF8StringEncoding));
  vertexFunction->setConstantValues(functionConstants.get());
  auto libVertexFunc =
      NS::TransferPtr(MTL4::LibraryFunctionDescriptor::alloc()->init());
  libVertexFunc->setLibrary(_pLibrary.get());
  libVertexFunc->setName(
      NS::String::string("pbr_vertex", NS::UTF8StringEncoding));
  vertexFunction->setFunctionDescriptor(libVertexFunc.get());

  auto fragmentFunction =
      NS::TransferPtr(MTL4::SpecializedFunctionDescriptor::alloc()->init());
  fragmentFunction->setSpecializedName(
      NS::String::string("pbr_fragment", NS::UTF8StringEncoding));
  fragmentFunction->setConstantValues(functionConstants.get());
  auto libFragmentFunc =
      NS::TransferPtr(MTL4::LibraryFunctionDescriptor::alloc()->init());
  libFragmentFunc->setLibrary(_pLibrary.get());
  libFragmentFunc->setName(
      NS::String::string("pbr_fragment", NS::UTF8StringEncoding));
  fragmentFunction->setFunctionDescriptor(libFragmentFunc.get());

  auto vertexDescriptor = NS::RetainPtr(
      MTK::MetalVertexDescriptorFromModelIO(pMesh->vertexDescriptor.get()));

  auto renderPipelineDescriptor =
      NS::TransferPtr(MTL4::RenderPipelineDescriptor::alloc()->init());
  renderPipelineDescriptor->setVertexFunctionDescriptor(vertexFunction.get());
  renderPipelineDescriptor->setFragmentFunctionDescriptor(
      fragmentFunction.get());
  renderPipelineDescriptor->setVertexDescriptor(vertexDescriptor.get());
  renderPipelineDescriptor->setRasterSampleCount(_rasterSampleCount);

  renderPipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(
      _colorPixelFormat);

  if (pMaterial->alphaMode == AlphaMode::Blend) {
    renderPipelineDescriptor->colorAttachments()->object(0)->setBlendingState(
        MTL4::BlendStateEnabled);
    renderPipelineDescriptor->colorAttachments()
        ->object(0)
        ->setSourceRGBBlendFactor(MTL::BlendFactorOne);
    renderPipelineDescriptor->colorAttachments()
        ->object(0)
        ->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
    renderPipelineDescriptor->colorAttachments()
        ->object(0)
        ->setRgbBlendOperation(MTL::BlendOperationAdd);
    renderPipelineDescriptor->colorAttachments()
        ->object(0)
        ->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
    renderPipelineDescriptor->colorAttachments()
        ->object(0)
        ->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
    renderPipelineDescriptor->colorAttachments()
        ->object(0)
        ->setAlphaBlendOperation(MTL::BlendOperationAdd);
  }

  NS::Error *pError = nullptr;
  return NS::TransferPtr(_pCompiler->newRenderPipelineState(
      renderPipelineDescriptor.get(), nullptr, &pError));
}

void Metal4Renderer::updateCamera() {
  auto orbitEntity = _pScene->rootEntity->childNamed("TreasureChest");
  if (!orbitEntity) {
    return;
  }
  const auto orbitRadius = 12.0f;
  // Берем позицию сундука (translation)
  const auto orbitCenter = orbitEntity->worldTransform().columns[3].xyz;

  // Считаем позицию камеры (вращение по кругу)
  const vector_float3 cameraPosition = {
      orbitCenter.x + orbitRadius * sinf(_time * 0.5f), 5.0f,
      orbitCenter.z + orbitRadius * cosf(_time * 0.5f)};

  // Считаем матрицу вида (World -> View)
  // Используем Right Hand, так как матрица проекции у вас тоже Right Hand
  const vector_float3 up = {0.0f, 1.0f, 0.0f};
  matrix_float4x4 viewMatrix =
      matrix_look_at_right_hand(cameraPosition, orbitCenter, up);

  // Камера хранит World Transform (Local -> World), поэтому инвертируем View
  // Matrix
  matrix_float4x4 worldMatrix = simd_inverse(viewMatrix);

  _camera.setTransform(worldMatrix);
}
// MTK::ViewDelegate
void Metal4Renderer::drawInMTKView(MTK::View *pView) {
  auto pPool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());

  static int frame = 0;
  if (frame++ % 60 == 0) {
  }

  if (!_hasPreparedResources) {
    return;
  }

  auto renderPassDescriptor = pView->currentMTL4RenderPassDescriptor();
  if (!renderPassDescriptor) {
    if (frame % 60 == 0)

      return;
  }
  if (_frameIndex >= kMaxFramesInFlight) {
    const auto valueToWait = _frameIndex - kMaxFramesInFlight;
    _pFrameCompletionEvent->waitUntilSignaledValue(valueToWait, 8);
  }

  const auto currentTime = CACurrentMediaTime();
  if (_lastRenderTime == 0)
    _lastRenderTime = currentTime;
  const auto deltaTime = std::min(currentTime - _lastRenderTime, 0.05);
  _lastRenderTime = currentTime;
  _time += deltaTime;

  updateCamera();

  _frameIndex++;

  // Получаем аллокатор и буфер констант для текущего кадра
  const auto frameIdx = _frameIndex % kMaxFramesInFlight;
  auto allocator = _pCommandAllocators[frameIdx].get();
  allocator->reset();

  RingBuffer *constantsBuffer = _pConstantBuffers[frameIdx];
  constantsBuffer->reset();

  _pCommandBuffer->beginCommandBuffer(allocator);

  const auto commandEncoder =
      _pCommandBuffer->renderCommandEncoder(renderPassDescriptor);

  commandEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);

  commandEncoder->setArgumentTable(_pVertexArgumentTable.get(),
                                   MTL::RenderStageVertex);
  commandEncoder->setArgumentTable(_pFragmentArgumentTable.get(),
                                   MTL::RenderStageFragment);

  // 1. Lights
  auto lightView = constantsBuffer->copy(_pScene->lights);
  _pFragmentArgumentTable->setAddress(lightView.gpuAddress(),
                                      fragmentBufferLights);

  // 2. Camera Matrices
  matrix_float4x4 viewMatrix = _camera.viewMatrix();
  CGSize drawableSize = pView->drawableSize();
  float aspectRatio = (float)(drawableSize.width / drawableSize.height);
  matrix_float4x4 projectionMatrix = _camera.projectionMatrix(aspectRatio);
  matrix_float4x4 viewProjectionMatrix =
      matrix_multiply(projectionMatrix, viewMatrix);

  FrameConstants frameConstants;
  frameConstants.viewMatrix = viewMatrix;
  frameConstants.projectionMatrix = projectionMatrix;
  frameConstants.viewProjectionMatrix = viewProjectionMatrix;
  frameConstants.environmentRotation = matrix_identity_float3x3;

  if (_pScene->pLightingEnvironment) {
    frameConstants.environmentIntensity =
        _pScene->pLightingEnvironment->intensity;
    frameConstants.specularEnvironmentMipCount =
        (uint32_t)_pScene->pLightingEnvironment->specularMipLevelCount;
  } else {
    frameConstants.environmentIntensity = 1.0f;
    frameConstants.specularEnvironmentMipCount = 1;
  }

  frameConstants.cameraPosition = _camera.position;
  frameConstants.activeLightCount = (uint32_t)_pScene->lights.size();

  // 3. Upload Frame Constants
  auto frameView = constantsBuffer->copy(frameConstants);

  _pVertexArgumentTable->setAddress(frameView.gpuAddress(),
                                    vertexBufferFrameConstants);
  _pFragmentArgumentTable->setAddress(frameView.gpuAddress(),
                                      fragmentBufferFrameConstants);

  // 4. Gather Draw Calls
  std::vector<DrawCall> drawCalls;
  drawCalls.reserve(100);

  _pScene->rootEntity->visitHierarchy([&](Entity *pEntity) {
    auto pModelEntity = dynamic_cast<ModelEntity *>(pEntity);
    if (pModelEntity && pModelEntity->mesh) {
      auto mesh = pModelEntity->mesh;

      simd_float4 localPos = {
          (float)pModelEntity->worldTransform().columns[3].x,
          (float)pModelEntity->worldTransform().columns[3].y,
          (float)pModelEntity->worldTransform().columns[3].z, 1.0f};
      simd_float4 modelViewPos4 = matrix_multiply(viewMatrix, localPos);
      simd_float3 modelViewPos = {modelViewPos4.x, modelViewPos4.y,
                                  modelViewPos4.z};

      for (auto &submesh : mesh->submeshes) {
        if (submesh.materialIndex < mesh->materials.size()) {
          Material *pMaterial = &mesh->materials[submesh.materialIndex];

          DrawCall dc;
          dc.mesh = mesh;
          dc.submesh = &submesh;
          dc.material = pMaterial;
          dc.modelTransform = pModelEntity->worldTransform();
          dc.modelViewPosition = modelViewPos;

          drawCalls.push_back(dc);
        }
      }
    }
  });

  // 5. Sort Draw Calls
  std::sort(drawCalls.begin(), drawCalls.end(),
            [](const DrawCall &a, const DrawCall &b) {
              int leftSort = a.material->relativeSortOrder();
              int rightSort = b.material->relativeSortOrder();

              if (leftSort > 0 && rightSort > 0) {
                // Transparent objects: Back-to-Front
                return a.modelViewPosition.z < b.modelViewPosition.z;
              }
              return leftSort < rightSort;
            });

  // 6. Bind IBL Textures
  if (auto ibl = _pScene->pLightingEnvironment) {
    _pCommandQueue->wait(ibl->readyEvent.get(), 1);

    _pFragmentArgumentTable->setTexture(
        ibl->diffuseCubeTexture.get()->gpuResourceID(),
        fragmentTextureDiffuseEnvironment);
    _pFragmentArgumentTable->setTexture(
        ibl->specularCubeTexture.get()->gpuResourceID(),
        fragmentTextureSpecularEnvironment);
    _pFragmentArgumentTable->setTexture(
        ibl->scaleAndBiasLookupTexture.get()->gpuResourceID(),
        fragmentTextureGGXLookup);
  }

  // 7. Execute Draw Calls
  for (const auto &dc : drawCalls) {
    commandEncoder->pushDebugGroup(
        NS::String::string(dc.mesh->name.c_str(), NS::UTF8StringEncoding));

    // Depth Stencil
    if (dc.material->alphaMode == AlphaMode::Opaque) {
      commandEncoder->setDepthStencilState(
          _pDepthStencilStates[(int)AlphaMode::Opaque].get());
    } else if (dc.material->alphaMode == AlphaMode::Mask) {
      commandEncoder->setDepthStencilState(
          _pDepthStencilStates[(int)AlphaMode::Mask].get());
    } else {
      commandEncoder->setDepthStencilState(
          _pDepthStencilStates[(int)AlphaMode::Blend].get());
    }

    // Bind Vertex Buffers
    for (size_t i = 0; i < dc.mesh->vertexBuffers.size(); ++i) {
      _pVertexArgumentTable->setAddress(
          dc.mesh->vertexBuffers[i].pBuffer->gpuAddress(), vertexBuffer0 + i);
    }

    // Instance Constants
    InstanceConstants instanceConstants;
    instanceConstants.modelMatrix = dc.modelTransform;

    // Normal Matrix (inverse transpose of upper-left 3x3)
    simd_float3x3 model3x3 = {simd_make_float3(dc.modelTransform.columns[0]),
                              simd_make_float3(dc.modelTransform.columns[1]),
                              simd_make_float3(dc.modelTransform.columns[2])};
    // Swift: .adjugate.transpose -> C++ equiv for orthogonal rotation/scale is
    // inverse(transpose) or just rotation part. Используем честный inverse
    // transpose
    instanceConstants.normalMatrix = simd_transpose(simd_inverse(model3x3));

    auto instanceView = constantsBuffer->copy(instanceConstants);
    _pVertexArgumentTable->setAddress(instanceView.gpuAddress(),
                                      vertexBufferInstanceConstants);
    _pFragmentArgumentTable->setAddress(instanceView.gpuAddress(),
                                        fragmentBufferInstanceConstants);

    // PSO
    if (dc.material->pRenderPipelineState) {
      commandEncoder->setRenderPipelineState(
          dc.material->pRenderPipelineState.get());
    }

    // Material Buffer
    if (dc.material->bufferView.pBuffer) {
      _pFragmentArgumentTable->setAddress(dc.material->bufferView.gpuAddress(),
                                          fragmentBufferMaterial);
    }

    // Draw
    commandEncoder->drawIndexedPrimitives(
        dc.submesh->primitiveType, dc.submesh->indexCount,
        dc.submesh->indexType, dc.submesh->indexBuffer.gpuAddress(),
        dc.submesh->indexBuffer.length);

    commandEncoder->popDebugGroup();
  }

  commandEncoder->endEncoding();

  _pCommandBuffer->endCommandBuffer();

  const auto drawable = pView->currentDrawable();

  _pCommandQueue->wait(drawable);
  const MTL4::CommandBuffer *pCmdBufs[] = {_pCommandBuffer.get()};
  _pCommandQueue->commit(pCmdBufs, 1);
  _pCommandQueue->signalDrawable(drawable);
  drawable->present();

  const auto valueToSignal = _frameIndex;
  _pCommandQueue->signalEvent(_pFrameCompletionEvent.get(), valueToSignal);
}

void Metal4Renderer::drawableSizeWillChange(MTK::View *pView, CGSize size) {
  // Обработка ресайза
}

void Metal4Renderer::configure(MTK::View *pView) {
  pView->setDevice(_pDevice.get());
  pView->setColorPixelFormat(_colorPixelFormat);
  pView->setClearColor(MTL::ClearColor(1.0f, 0.0f, 0.0f, 1.0f));
  pView->setDepthStencilPixelFormat(_depthStencilPixelFormat);
  pView->setSampleCount(_rasterSampleCount);
  pView->setDelegate(this);

  CA::MetalLayer *pLayer = (CA::MetalLayer *)pView->layer();
  _pCommandQueue->addResidencySet(pLayer->residencySet());
}
