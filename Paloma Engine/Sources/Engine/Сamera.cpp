//
//  Сamera.cpp
//  Paloma Engine
//
//  Created by Artem on 20.01.2026.
//

#include "Camera.hpp"

using namespace simd;

void PerspectiveCamera::setTransform(const matrix_float4x4 &transform) {
  // Извлечение позиции (последний столбец)
  this->position = transform.columns[3].xyz;

  // Извлечение ориентации (кватернион из вращения 3x3)
  matrix_float3x3 rotationMatrix;
  rotationMatrix.columns[0] = transform.columns[0].xyz;
  rotationMatrix.columns[1] = transform.columns[1].xyz;
  rotationMatrix.columns[2] = transform.columns[2].xyz;

  this->orientation = simd_quaternion(rotationMatrix);
}

matrix_float4x4 PerspectiveCamera::transform() const {
  // Создаем матрицу трансформации из позиции и ориентации

  // 1. Вращение из кватерниона
  matrix_float3x3 rot = simd_matrix3x3(this->orientation);

  // 2. Сборка матрицы 4x4
  matrix_float4x4 mat;
  mat.columns[0] = make_float4(rot.columns[0], 0.0f);
  mat.columns[1] = make_float4(rot.columns[1], 0.0f);
  mat.columns[2] = make_float4(rot.columns[2], 0.0f);
  mat.columns[3] = make_float4(this->position, 1.0f);

  return mat;
}

matrix_float4x4 PerspectiveCamera::viewMatrix() const {
  // View Matrix - это инвертированная матрица трансформации камеры.
  matrix_float4x4 tf = this->transform();
  return simd_inverse(tf);
}

matrix_float4x4 PerspectiveCamera::projectionMatrix(float aspectRatio) const {
  float fovRadians = this->fieldOfViewDegrees * (M_PI / 180.0f);
  return matrix_perspective_right_hand(fovRadians, aspectRatio, this->nearZ,
                                       this->farZ);
}
