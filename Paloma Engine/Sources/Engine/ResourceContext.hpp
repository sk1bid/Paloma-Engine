#pragma once

#include "Material.hpp"
#include "Mesh.hpp"
#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>
#include <ModelIO/ModelIO.hpp>
#include <map>
#include <memory>
#include <vector>

class ResourceContext {
public:
  enum class TextureSemantic { Raw, Color };

  // Храним ссылки на ресурсы, чтобы они не удалились из памяти
  std::vector<NS::SharedPtr<MTL::Resource>> resources;

  ResourceContext(MTL::Device *pDevice);

  // Методы конвертации (Twins of Swift)
  std::shared_ptr<Mesh> convert(MDL::Mesh *mdlMesh);
  Material convert(MDL::Material *mdlMaterial);
  NS::SharedPtr<MTL::Texture> convert(MDL::Texture *mdlTexture,
                                      TextureSemantic semantic);

private:
  MTL::Device *_pDevice;
  NS::SharedPtr<MTK::TextureLoader> _pTextureLoader;

  // Опции для загрузки текстур (словарь в Swift -> NSDictionary в ObjC/C++)
  NS::SharedPtr<NS::Dictionary> _dataTextureOptions;
  NS::SharedPtr<NS::Dictionary> _colorTextureOptions;

  // Кэши (ObjectIdentifier в Swift -> указатель в C++)
  std::map<MDL::Texture *, NS::SharedPtr<MTL::Texture>> _textureCache;
  std::map<MDL::Material *, Material> _materialCache;
};
