//
// math3d.h
// by Naomi Peori <naomi@peori.ca>
//

#pragma once
#include <array>

// ============================================================
//  Column-major 4x4 matrix math
//
//  Memory layout: m[col * 4 + row]
//  All operations use a right-handed coordinate system.
//  Perspective maps Z to [0, 1] (Metal/DX12 NDC convention).
// ============================================================

struct Vec3 {
  float x, y, z;
};

struct alignas(16) Mat4 {
  std::array<float, 16> m = {}; // column-major: m[col * 4 + row]

  constexpr float &operator[](int i) { return m[i]; }
  constexpr float operator[](int i) const { return m[i]; }

  float *data() { return m.data(); }
  const float *data() const { return m.data(); }

  Mat4 operator*(const Mat4 &b) const;

  static constexpr Mat4 identity() {
    Mat4 r;
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
  }
};

/// Right-handed perspective projection, Z mapped to [0, 1] (Metal NDC).
[[nodiscard]] Mat4 math3d_perspective(float fovy_rad, float aspect, float near_z, float far_z);

/// Look-at view matrix (right-handed, eye looking toward target).
[[nodiscard]] Mat4 math3d_look_at(Vec3 eye, Vec3 target, Vec3 up);

/// Rotation matrix around the Y axis (right-handed).
[[nodiscard]] Mat4 math3d_rotation_y(float angle);

/// Rotation matrix around the X axis (right-handed).
[[nodiscard]] Mat4 math3d_rotation_x(float angle);

/// Rotation matrix around the Z axis (right-handed).
[[nodiscard]] Mat4 math3d_rotation_z(float angle);

/// Matrix multiplication a * b (column-major).
[[nodiscard]] Mat4 math3d_mul(const Mat4 &a, const Mat4 &b);
