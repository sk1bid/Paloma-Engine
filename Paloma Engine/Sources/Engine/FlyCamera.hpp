//
//  FlyCamera.hpp
//  Paloma Engine
//
//  Created by Artem on 20.01.2026.
//
#include "AAPLMathUtilities.h"
#include "InputManager.hpp"
#include "iostream"
#include "simd/simd.h"

class FlyCamera {
public:
  simd_float3 position = {0, 5, 10};
  float yaw = 0.0f;
  float pitch = 0.0f;

  float moveSpeed = 5.0f;
  float lookSpeed = 0.003f;

  void update(float deltaTime, const InputManager &input);
  void lookAt(simd_float3 target);
  matrix_float4x4 viewMatrix() const;

private:
  simd_float3 forward() const;
  simd_float3 right() const;
};
