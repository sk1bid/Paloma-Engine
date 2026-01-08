//
//  Camera.hpp
//  Paloma Engine
//
//  Created by Artem on 08.01.2026.

#pragma once

#include "Types.hpp"
#include "AAPLMathUtilities.h"
#include <simd/simd.h>

namespace Paloma {

class Camera {
public:
    // -- Transform --
    float3 position = {0.0f, 0.0f, 8.0f};
    float3 target = {0.0f, 0.0f, 0.0f};
    float3 up = {0.0f, 1.0f, 0.0f};
    
    // -- Projection --
    float fov = 65.0f; // degrees
    float nearZ = 0.1f;
    float farZ = 100.0f;
    float aspectRatio = 1.0f;
    
    // -- Computed Matrices --
    simd_float4x4 viewMatrix() const {
        return matrix_look_at_right_hand(position, target, up);
    }
    
    simd_float4x4 projectionMatrix() const {
        float fovRadians = fov * (M_PI / 180.0f);
        return matrix_perspective_right_hand(fovRadians, aspectRatio, nearZ, farZ);
    }
    
    // -- Helpers --
    float3 forward() const {
        return simd_normalize(target - position);
    }
    
    float3 right() const {
        return simd_normalize(simd_cross(forward(), up));
    }
};

}
