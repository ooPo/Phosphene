//
// math3d.cpp
// by Naomi Peori <naomi@peori.ca>
//

#include "phosphene/math3d.h"
#include <cmath>

Mat4 Mat4::operator*(const Mat4 &b) const { return math3d_mul(*this, b); }

Mat4 math3d_perspective(float fovy_rad, float aspect, float near_z, float far_z) {
  float f = 1.0f / tanf(fovy_rad * 0.5f);
  float r = 1.0f / (near_z - far_z);
  Mat4 m;
  m[0]  = f / aspect;
  m[5]  = f;
  m[10] = far_z * r;
  m[11] = -1.0f;
  m[14] = near_z * far_z * r;
  return m;
}

Mat4 math3d_look_at(Vec3 eye, Vec3 target, Vec3 up) {
  float fx = eye.x - target.x, fy = eye.y - target.y, fz = eye.z - target.z;
  float fl = sqrtf(fx * fx + fy * fy + fz * fz);
  fx /= fl;
  fy /= fl;
  fz /= fl;

  float rx = up.y * fz - up.z * fy;
  float ry = up.z * fx - up.x * fz;
  float rz = up.x * fy - up.y * fx;
  float rl = sqrtf(rx * rx + ry * ry + rz * rz);
  rx /= rl;
  ry /= rl;
  rz /= rl;

  float vx = fy * rz - fz * ry;
  float vy = fz * rx - fx * rz;
  float vz = fx * ry - fy * rx;

  Mat4 m;
  m[0]  = rx;
  m[1]  = ry;
  m[2]  = rz;
  m[4]  = vx;
  m[5]  = vy;
  m[6]  = vz;
  m[8]  = fx;
  m[9]  = fy;
  m[10] = fz;
  m[12] = -(rx * eye.x + ry * eye.y + rz * eye.z);
  m[13] = -(vx * eye.x + vy * eye.y + vz * eye.z);
  m[14] = -(fx * eye.x + fy * eye.y + fz * eye.z);
  m[15] = 1.0f;
  return m;
}

Mat4 math3d_rotation_y(float angle) {
  float c = cosf(angle), s = sinf(angle);
  Mat4 m;
  m[0]  = c;
  m[2]  = -s;
  m[5]  = 1.0f;
  m[8]  = s;
  m[10] = c;
  m[15] = 1.0f;
  return m;
}

Mat4 math3d_rotation_x(float angle) {
  float c = cosf(angle), s = sinf(angle);
  Mat4 m;
  m[0]  = 1.0f;
  m[5]  = c;
  m[6]  = s;
  m[9]  = -s;
  m[10] = c;
  m[15] = 1.0f;
  return m;
}

Mat4 math3d_rotation_z(float angle) {
  float c = cosf(angle), s = sinf(angle);
  Mat4 m;
  m[0]  = c;
  m[1]  = s;
  m[4]  = -s;
  m[5]  = c;
  m[10] = 1.0f;
  m[15] = 1.0f;
  return m;
}

Mat4 math3d_mul(const Mat4 &a, const Mat4 &b) {
  Mat4 out;
  for (int col = 0; col < 4; ++col) {
    for (int row = 0; row < 4; ++row) {
      float sum = 0.0f;
      for (int k = 0; k < 4; ++k) {
        sum += a[k * 4 + row] * b[col * 4 + k];
      }
      out[col * 4 + row] = sum;
    }
  }
  return out;
}
