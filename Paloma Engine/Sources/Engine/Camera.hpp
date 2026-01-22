//
//  Camera.hpp
//  Paloma Engine
//
//  Created by Artem on 20.01.2026.
//

#pragma once

#include <simd/simd.h>
#include "AAPLMathUtilities.h"

class PerspectiveCamera {
public:
    PerspectiveCamera() = default;

    simd::float3 position = {0.0f, 0.0f, 0.0f};
    
    simd::quatf orientation = simd::quatf(1.0f, 0.0f, 0.0f, 0.0f);

    float fieldOfViewDegrees = 60.0f;
    
    float nearZ = 0.1f;
    float farZ = 1000.0f;

    void setTransform(const matrix_float4x4& transform);
    
    matrix_float4x4 transform() const;

    matrix_float4x4 viewMatrix() const;

    matrix_float4x4 projectionMatrix(float aspectRatio) const;
};
