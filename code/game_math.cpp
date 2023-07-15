#include <math.h>

#define Pi32 3.141592

s32 round_f32_to_s32(f32 value) {
  s32 r = (s32)(value + 0.555f);
  return r;
}

//
// Note: Vec2
//
struct Vec2 {
  union {
    struct { f32 x, y; };
    struct { f32 width, height; };
    struct { f32 min, max; };
  }; 
};

Vec2 vec2(f32 x, f32 y) {
  return {x, y};
}

Vec2 vec2(f32 angle) {
  return {cosf(angle), sinf(angle)};
}

f32 vec2_angle(Vec2 v) {
  f32 r = atan2f(v.y, v.x);
  return r;
}

f32 vec2_length(Vec2 v) { return sqrtf(v.x*v.x + v.y*v.y); }

Vec2 vec2_normalize(Vec2 v) {
  f32 l = sqrtf(v.x*v.x + v.y*v.y);
  if(l == 0.0f) return {0, 0};
  return {v.x/l, v.y/l};
}

Vec2 vec2_rotate(Vec2 v, f32 angle) {
  Vec2 r = {
     v.x*cosf(angle) - v.y*sinf(angle),
     v.x*sinf(angle) + v.y*cosf(angle)
  };
    
    return r;
}

Vec2 vec2_perp(Vec2 v) {
  Vec2 r = {v.y, -v.x};
  return r;
}


Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
void operator+=(Vec2 &a, Vec2 b) { a.x += b.x; a.y += b.y; }

Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
void operator-=(Vec2 &a, Vec2 b) { a.x -= b.x; a.y -= b.y; }

Vec2 operator*(Vec2 v, f32 s) { return {v.x*s, v.y*s}; }
Vec2 operator*(f32 s, Vec2 v) { return {v.x*s, v.y*s}; }


Vec2 vec2_lerp(Vec2 a, Vec2 b, f32 t) {
  Vec2 r = a + (b - a)*t;
  return r;
}

//
// Note: Vec3
//
struct Vec3 {
  f32 x, y, z;
};

//
// Note: Vec4
//
struct Vec4 {
  union{
    struct { f32 x, y, z, w; };
    struct { f32 r, g, b, a; };
    struct { Vec2 pos, dim;  };
  }; 
};

Vec4 vec4(f32 r, f32 g, f32 b, f32 a) {
  return {r, g, b, a};
} 

Vec4 vec4(Vec2 pos, Vec2 dim) {
  Vec4 r = {};
  r.pos = pos;
  r.dim = dim;
  return r;
}

Vec4 vec4(u32 color) {
  Vec4 c = {};
  c.r = (f32)((color >> 16)&0xFF)/255.f;
  c.g = (f32)((color >> 8)&0xFF)/255.f;
  c.b = (f32)((color >> 0)&0xFF)/255.f;
  c.a = (f32)((color >> 24)&0xFF)/255.f;
  return c;
}



Vec4 vec4_fade_alpha(Vec4 v, f32 a) { return {v.r, v.g, v.b, a}; }

u32 pack_color_argb(Vec4 v) {
  u32 result = 0;

  u32 a32 = (u32)(v.a*255.0f);
  u32 r32 = (u32)(v.r*255.0f);
  u32 g32 = (u32)(v.g*255.0f);
  u32 b32 = (u32)(v.b*255.0f);
  
  result = ((a32 << 24) | (r32 << 16) | (g32 << 8) | (b32 << 0));
  return result;
}

u32 pack_color_argb(f32 r, f32 g, f32 b, f32 a) {
  u32 result = 0;

  u32 a32 = (u32)(a*255.0f);
  u32 r32 = (u32)(r*255.0f);
  u32 g32 = (u32)(g*255.0f);
  u32 b32 = (u32)(b*255.0f);
  
  result = ((a32 << 24) | (r32 << 16) | (g32 << 8) | (b32 << 0));
  return result;
}

//
// Imporant: Our matrices are in row-major order.
//


//
// Note: Matrix2
//
struct Mat2 {
  union {
    struct {
      f32 m00, m01;
      f32 m10, m11;
    };
    f32 m[4];
  };
};

Vec2 operator*(Mat2 m, Vec2 v) {
  Vec2 r = {
    v.x*m.m00 + v.y*m.m01,
    v.x*m.m10 + v.y*m.m11,
  };
  return r;
}

//
// Note: Matrix4
//
struct Mat4 {
  union {
    struct {
      f32 m00, m01, m02, m03;
      f32 m10, m11, m12, m13;
      f32 m20, m21, m22, m23;
      f32 m30, m31, m32, m33;
    };
    f32 m[16];
  };
};

Mat4 mat4_identity() {
  Mat4 mat = {1, 0, 0, 0,
              0, 1, 0, 0,
              0, 0, 1, 0,
              0, 0, 0, 1};
  return mat;
}

Vec4 mat4_mult(Mat4 a, Vec4 v) {
  Vec4 r = {};

  r.x = a.m00*v.x + a.m01*v.y + a.m02*v.z + a.m03*v.w;
  r.y = a.m10*v.x + a.m11*v.y + a.m12*v.z + a.m13*v.w;
  r.z = a.m20*v.x + a.m21*v.y + a.m22*v.z + a.m23*v.w;
  r.w = a.m30*v.x + a.m31*v.y + a.m32*v.z + a.m33*v.w;
  
  return r;
}

Mat4 mat4_mult(Mat4 a, Mat4 b) {
  Mat4 r = {};

  r.m00 = a.m00*b.m00 + a.m01*b.m10 + a.m02*b.m20 + a.m03*b.m30;
  r.m01 = a.m00*b.m01 + a.m01*b.m11 + a.m02*b.m21 + a.m03*b.m31;
  r.m02 = a.m00*b.m02 + a.m01*b.m12 + a.m02*b.m22 + a.m03*b.m32;
  r.m03 = a.m00*b.m03 + a.m01*b.m13 + a.m02*b.m23 + a.m03*b.m33;

  r.m10 = a.m10*b.m00 + a.m11*b.m10 + a.m12*b.m20 + a.m13*b.m30;
  r.m11 = a.m10*b.m01 + a.m11*b.m11 + a.m12*b.m21 + a.m13*b.m31;
  r.m12 = a.m10*b.m02 + a.m11*b.m12 + a.m12*b.m22 + a.m13*b.m32;
  r.m13 = a.m10*b.m03 + a.m11*b.m13 + a.m12*b.m23 + a.m13*b.m33;

  r.m20 = a.m20*b.m00 + a.m21*b.m10 + a.m22*b.m20 + a.m23*b.m30;
  r.m21 = a.m20*b.m01 + a.m21*b.m11 + a.m22*b.m21 + a.m23*b.m31;
  r.m22 = a.m20*b.m02 + a.m21*b.m12 + a.m22*b.m22 + a.m23*b.m32;
  r.m23 = a.m20*b.m03 + a.m21*b.m13 + a.m22*b.m23 + a.m23*b.m33;

  r.m30 = a.m30*b.m00 + a.m31*b.m10 + a.m32*b.m20 + a.m33*b.m30;
  r.m31 = a.m30*b.m01 + a.m31*b.m11 + a.m32*b.m21 + a.m33*b.m31;
  r.m32 = a.m30*b.m02 + a.m31*b.m12 + a.m32*b.m22 + a.m33*b.m32;
  r.m33 = a.m30*b.m03 + a.m31*b.m13 + a.m32*b.m23 + a.m33*b.m33;
  
  return r;
}

Mat4 mat4_ortho_right_handed(f32 l, f32 r, f32 t, f32 b, f32 n, f32 f) {
  
  Mat4 result = {
    2/(r - l), 0,         0,         -(r + l)/(r - l),
    0,        -2/(b - t), 0,          (b + t)/(b - t),
    0,         0,        -2/(f - n), -(f + n)/(f - n),
    0,         0,         0,          1
  };

  return result;
}
