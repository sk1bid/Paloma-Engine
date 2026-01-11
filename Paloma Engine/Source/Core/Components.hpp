//
//  Components.hpp
//  Paloma Engine
//
//  Created by Artem on 10.01.2026.
//

#pragma once
#include <simd/simd.h>


namespace MTL{
class Texture;
}

namespace Paloma {
class Mesh;
}


struct TransformComponent{
    simd_float3 position;
    simd_float3 rotation;
    simd_float3 scale;
};

struct MeshComponent {
    Paloma::Mesh* mesh;
    uint64_t materialAddress;
};


