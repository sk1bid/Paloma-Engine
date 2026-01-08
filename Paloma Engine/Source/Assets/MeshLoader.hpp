//
//  MeshLoader.hpp
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#pragma once

#include "Mesh.hpp"
#include <Metal/Metal.hpp>

namespace Paloma {

// -- MeshLoader - loading 3D models with ModelIO --

namespace MeshLoader {

///Load mesh from file
///@param device - Metal device
///@param path - path to file (bundle or full path)
///@return Mesh*
Mesh* loadFromFile(MTL::Device* device, const char* path);

/// Load primitive
/// @param name - "box", "sphere", "plane"
Mesh* loadPrimitive(MTL::Device* device, const char* name);
}
}
