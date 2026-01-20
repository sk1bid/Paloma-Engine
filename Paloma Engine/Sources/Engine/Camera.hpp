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

    // Свойства (var position, orientation...)
    simd::float3 position = {0.0f, 0.0f, 0.0f};
    
    // В Swift orientation - это кватернион (simd_quatd).
    // В C++ используем simd::quatf
    simd::quatf orientation = simd::quatf(1.0f, 0.0f, 0.0f, 0.0f); // Identity (w, x, y, z)

    // Field of View в градусах (как в Swift .degrees(60))
    float fieldOfViewDegrees = 60.0f;
    
    float nearZ = 0.1f;
    float farZ = 1000.0f;

    // func setTransform(_ transform: AffineTransform3D)
    // В C++ AffineTransform3D обычно передается как матрица 4x4
    void setTransform(const matrix_float4x4& transform);
    
    // var transform: simd_float4x4
    matrix_float4x4 transform() const;

    // func viewMatrix() -> simd_float4x4
    matrix_float4x4 viewMatrix() const;

    // func projectionMatrix(aspectRatio: Double) -> simd_float4x4
    matrix_float4x4 projectionMatrix(float aspectRatio) const;
};
