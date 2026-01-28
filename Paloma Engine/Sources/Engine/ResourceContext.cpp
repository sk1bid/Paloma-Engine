#include "ResourceContext.hpp"
#include "AAPLMathUtilities.h"
#include <iostream>

ResourceContext::ResourceContext(MTL::Device *pDevice) : _pDevice(pDevice) {
  _pTextureLoader = NS::TransferPtr(MTK::TextureLoader::alloc()->init(pDevice));

  auto keySRGB = MTK::TextureLoaderOptionSRGB;
  auto valSRGBFalse = NS::Number::number(false);

  auto keyMips = MTK::TextureLoaderOptionGenerateMipmaps;
  auto valMipsTrue = NS::Number::number(true);

  auto keyStorage = MTK::TextureLoaderOptionTextureStorageMode;
  auto valStorage = NS::Number::number((NS::UInteger)MTL::StorageModePrivate);

  NS::Object *keysData[] = {keySRGB, keyMips, keyStorage};
  NS::Object *valsData[] = {valSRGBFalse, valMipsTrue, valStorage};

  _dataTextureOptions =
      NS::RetainPtr(NS::Dictionary::dictionary(valsData, keysData, 3));

  // Setup Color Texture Options
  auto valSRGBTrue = NS::Number::number(true);
  NS::Object *valsColor[] = {valSRGBTrue, valMipsTrue, valStorage};

  _colorTextureOptions =
      NS::RetainPtr(NS::Dictionary::dictionary(valsColor, keysData, 3));
}

NS::SharedPtr<MTL::Texture> ResourceContext::convert(MDL::Texture *mdlTexture,
                                                     TextureSemantic semantic) {
  auto it = _textureCache.find(mdlTexture);
  if (it != _textureCache.end()) {
    return it->second;
  }

  NS::Dictionary *options = (semantic == TextureSemantic::Raw)
                                ? _dataTextureOptions.get()
                                : _colorTextureOptions.get();

  NS::Error *pError = nullptr;
  MTL::Texture *rawTexture =
      _pTextureLoader->newTexture(mdlTexture, options, &pError);

  if (!rawTexture) {
    if (pError) {
      std::cerr << "Failed to load texture: "
                << pError->localizedDescription()->cString(
                       NS::UTF8StringEncoding)
                << std::endl;
    }
    return nullptr;
  }

  NS::SharedPtr<MTL::Texture> texture = NS::TransferPtr(rawTexture);

  if (semantic == TextureSemantic::Color &&
      texture->pixelFormat() != MTL::PixelFormatRGBA8Unorm_sRGB) {
    MTL::Texture *view =
        texture->newTextureView(MTL::PixelFormatRGBA8Unorm_sRGB);
    if (view) {
      texture = NS::TransferPtr(view);
    }
  }

  resources.push_back(texture);
  _textureCache[mdlTexture] = texture;

  return texture;
}

Material ResourceContext::convert(MDL::Material *mdlMaterial) {
  auto it = _materialCache.find(mdlMaterial);
  if (it != _materialCache.end()) {
    return it->second;
  }

  Material material;
  if (mdlMaterial->name()) {
    material.name = NS::TransferPtr(mdlMaterial->name()->copy());
  } else {
    material.name =
        NS::RetainPtr(NS::String::string("Material", NS::UTF8StringEncoding));
  }

  auto getProp =
      [&](MDL::MaterialSemantic semantic) -> MDL::MaterialProperty * {
    return mdlMaterial->propertyWithSemantic(semantic);
  };

  if (auto prop = getProp(MDL::MaterialSemanticBaseColor)) {
    if (prop->type() == MDL::MaterialPropertyTypeTexture) {
      if (auto sampler = prop->textureSamplerValue()) {
        if (auto tex = sampler->texture()) {
          material.baseColor.pTexture = convert(tex, TextureSemantic::Color);
        }
      }
      material.baseColor.factor = {1, 1, 1};
    } else if (prop->type() == MDL::MaterialPropertyTypeFloat3) {
      vector_float3 val = prop->float3Value();
      material.baseColor.factor = val;
    }
  }

  if (auto prop = getProp(MDL::MaterialSemanticRoughness)) {
    if (prop->type() == MDL::MaterialPropertyTypeTexture) {
      if (auto sampler = prop->textureSamplerValue()) {
        if (auto tex = sampler->texture()) {
          material.roughness.pTexture = convert(tex, TextureSemantic::Raw);
        }
      }
    } else if (prop->type() == MDL::MaterialPropertyTypeFloat) {
      material.roughness.factor = prop->floatValue();
    }
  }

  if (auto prop = getProp(MDL::MaterialSemanticMetallic)) {
    if (prop->type() == MDL::MaterialPropertyTypeTexture) {
      if (auto sampler = prop->textureSamplerValue()) {
        if (auto tex = sampler->texture()) {
          material.metalness.pTexture = convert(tex, TextureSemantic::Raw);
        }
      }
      material.metalness.factor = 1.0f;
    } else if (prop->type() == MDL::MaterialPropertyTypeFloat) {
      material.metalness.factor = prop->floatValue();
    }
  }

  if (auto prop = getProp(MDL::MaterialSemanticTangentSpaceNormal)) {
    if (prop->type() == MDL::MaterialPropertyTypeTexture) {
      if (auto sampler = prop->textureSamplerValue()) {
        if (auto tex = sampler->texture()) {
          material.normal.pTexture = convert(tex, TextureSemantic::Raw);
        }
      }
    }
  }

  if (auto prop = getProp(MDL::MaterialSemanticEmission)) {
    if (prop->type() == MDL::MaterialPropertyTypeTexture) {
      if (auto sampler = prop->textureSamplerValue()) {
        if (auto tex = sampler->texture()) {
          material.emissive.pTexture = convert(tex, TextureSemantic::Color);
        }
      }
      material.emissive.factor = {1, 1, 1};
    } else if (prop->type() == MDL::MaterialPropertyTypeFloat3) {
      material.emissive.factor = prop->float3Value();
    }
  }

  if (auto prop = getProp(MDL::MaterialSemanticAmbientOcclusion)) {
    if (prop->type() == MDL::MaterialPropertyTypeTexture) {
      if (auto sampler = prop->textureSamplerValue()) {
        if (auto tex = sampler->texture()) {
          material.occlusion.pTexture = convert(tex, TextureSemantic::Raw);
        }
      }
    }
  }
  if (auto prop = getProp(MDL::MaterialSemanticAmbientOcclusionScale)) {
    if (prop->type() == MDL::MaterialPropertyTypeFloat) {
      material.occlusion.factor = prop->floatValue();
    }
  }

  if (auto prop = getProp(MDL::MaterialSemanticOpacity)) {
    if (prop->type() == MDL::MaterialPropertyTypeTexture) {
      if (auto sampler = prop->textureSamplerValue()) {
        if (auto tex = sampler->texture()) {
          material.opacity.pTexture = convert(tex, TextureSemantic::Raw);
        }
      }
      material.alphaMode = AlphaMode::Blend;
    }
  }

  _materialCache[mdlMaterial] = material;
  return material;
}

std::shared_ptr<Mesh> ResourceContext::convert(MDL::Mesh *mdlMesh) {
  NS::Error *pError = nullptr;

  MTK::Mesh *mtkMesh = MTK::Mesh::alloc()->init(mdlMesh, _pDevice, &pError);

  if (!mtkMesh) {
    std::cerr << "Failed to create MTKMesh: "
              << (pError ? pError->localizedDescription()->cString(
                               NS::UTF8StringEncoding)
                         : "Unknown error")
              << std::endl;
    return nullptr;
  }

  std::vector<BufferView> vertexBuffers;
  NS::Array *mtkBuffers = mtkMesh->vertexBuffers();

  for (NS::UInteger i = 0; i < mtkBuffers->count(); ++i) {
    MTK::MeshBuffer *mtkBuffer = mtkBuffers->object<MTK::MeshBuffer>(i);
    MTL::Buffer *mtlBuffer = mtkBuffer->buffer();

    NS::String *label =
        NS::String::string("Vertex Attributes", NS::UTF8StringEncoding);
    mtlBuffer->setLabel(label);

    resources.push_back(NS::RetainPtr(mtlBuffer));
    vertexBuffers.push_back(
        {mtlBuffer, mtkBuffer->offset(), mtkBuffer->length()});
  }

  Material defaultMaterial;
  defaultMaterial.baseColor.factor = {0.18f, 0.18f, 0.18f};
  defaultMaterial.metalness.factor = 0.0f;
  defaultMaterial.roughness.factor = 0.8f;

  std::vector<Submesh> submeshes;
  std::vector<Material> materials;

  NS::Array *mtkSubmeshes = mtkMesh->submeshes();
  NS::Array *mdlSubmeshes = mdlMesh->submeshes();


  for (NS::UInteger i = 0; i < mtkSubmeshes->count(); ++i) {
    MTK::Submesh *mtkSubmesh = mtkSubmeshes->object<MTK::Submesh>(i);
    MTK::MeshBuffer *mtkIdxBuffer = mtkSubmesh->indexBuffer();
    MTL::Buffer *mtlIdxBuf = mtkIdxBuffer->buffer();

    resources.push_back(NS::RetainPtr(mtlIdxBuf));

    if (i < mdlSubmeshes->count()) {
      MDL::Submesh *mdlOriginal = mdlSubmeshes->object<MDL::Submesh>(i);
      if (MDL::Material *mdlMat = mdlOriginal->material()) {
        materials.push_back(convert(mdlMat));
      } else {
        materials.push_back(defaultMaterial);
      }
    } else {
      materials.push_back(defaultMaterial);
    }

    submeshes.push_back(
        Submesh(mtkSubmesh->primitiveType(),
                {mtlIdxBuf, mtkIdxBuffer->offset(), mtkIdxBuffer->length()},
                mtkSubmesh->indexType(), mtkSubmesh->indexCount(), (int)i));
  }

  NS::String *nameObj = ((MDL::Named *)mdlMesh)->name();
  std::string name =
      nameObj ? nameObj->cString(NS::UTF8StringEncoding) : "Mesh";

  auto vertexDescriptor = NS::RetainPtr(mtkMesh->vertexDescriptor());

  return std::make_shared<Mesh>(name, vertexBuffers, mtkMesh->vertexCount(),
                                vertexDescriptor, submeshes, materials);
}
