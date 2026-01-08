//
//  TextureLoader.hpp
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#pragma once

#include <Metal/Metal.hpp>

namespace Paloma {
namespace TextureLoader {

/// Load texture from flie
/// @param device - Metal device
/// @param path - path to file
/// @param sRGB - use sRGB color space
MTL::Texture* loadFromFile(MTL::Device* device, const char* path, bool sRGB = true);

/// Load texture from bundle
MTL::Texture* loadFromBundle(MTL::Device* device, const char* path, bool sRGB = true);
}
}
