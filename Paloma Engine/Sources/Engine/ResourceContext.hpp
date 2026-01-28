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

  // Keep references to resources to prevent deallocation
  std::vector<NS::SharedPtr<MTL::Resource>> resources;

  ResourceContext(MTL::Device *pDevice);

  // Conversion methods
  std::shared_ptr<Mesh> convert(MDL::Mesh *mdlMesh);
  Material convert(MDL::Material *mdlMaterial);
  NS::SharedPtr<MTL::Texture> convert(MDL::Texture *mdlTexture,
                                      TextureSemantic semantic);

private:
  MTL::Device *_pDevice;
  NS::SharedPtr<MTK::TextureLoader> _pTextureLoader;

  NS::SharedPtr<NS::Dictionary> _dataTextureOptions;
  NS::SharedPtr<NS::Dictionary> _colorTextureOptions;

  // Caches
  std::map<MDL::Texture *, NS::SharedPtr<MTL::Texture>> _textureCache;
  std::map<MDL::Material *, Material> _materialCache;
};
