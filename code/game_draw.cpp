
//
// Game Draw
//

Color rl_color(f32 r, f32 g, f32 b, f32 a) {
  Color c = {(u8)(r*255.0f), (u8)(g*255.0f), (u8)(b*255.0f), (u8)(a*255.0f)};
  return c;
}

Color rl_color(Vec4 c) {
  Color r = {(u8)(c.r*255.0f), (u8)(c.g*255.0f), (u8)(c.b*255.0f), (u8)(c.a*255.0f)};
  return r;
}

Vector2 rl_vec2(Vec2 v) {
  Vector2 r = {v.x, v.y};
  return r;
}

Rectangle rl_rec(Vec2 pos, Vec2 dim) {
  Rectangle rec = {pos.x, pos.y, dim.width, dim.height};
  return rec;
}

Rectangle rl_rec(Vec4 r) {
  Rectangle rec = {r.x, r.y, r.z, r.w};
  return rec;
}

//
// Line 
//
void draw_line(Vec2 start, Vec2 end, f32 thickness, Vec4 color) {
  DrawLineEx(rl_vec2(start), rl_vec2(end), thickness, rl_color(color));
}

//
// Triangle
//
void draw_triangle(Vec2 v0, Vec2 v1, Vec2 v2, Vec4 color) {
  DrawTriangle(rl_vec2(v0), rl_vec2(v1), rl_vec2(v2), rl_color(color));
}


//
// Colored and textured quad
//
void draw_quad(Vec2 pos, Vec2 dim, f32 rot, Vec4 color) {
  Vec2 half_dim = dim*0.5f;
  f32 rot_deg = (rot*180.0f)/Pi32;
  DrawRectanglePro(rl_rec(pos + half_dim, dim), rl_vec2(half_dim), rot_deg, rl_color(color));
}
void draw_quad(Vec2 pos, Vec2 dim, Vec4 color) { draw_quad(pos, dim, 0.0f, color); }

// Quad outline
void draw_quad_outline(Vec2 pos, Vec2 dim, f32 thickness, Vec4 color) {
  Color col = rl_color(color);
  DrawRectangleLinesEx({pos.x, pos.y, dim.width, dim.height}, thickness, col);
}

void draw_quad(Texture2D texture, Vec4 src, Vec4 dest, Vec2 orig, f32 rot, Vec4 color) {
  Vec2 half_dim = dest.dim*0.5f;
  dest.pos += half_dim;
  f32 rot_deg = (rot*180.0f)/Pi32;
  
  DrawTexturePro(texture, rl_rec(src), rl_rec(dest), rl_vec2(half_dim), rot_deg, rl_color(color));
}

void draw_quad(Texture2D texture, Vec2 pos, Vec2 dim, f32 rot, Vec4 color) {
  Vec4 src  = vec4(0, 0, texture.width, texture.height);
  Vec4 dest = vec4(pos, dim);
  Vec2 orig = pos;
  draw_quad(texture, src, dest, orig, rot, color);
}

void draw_quad(Texture2D texture, Vec2 pos, Vec2 dim, Vec4 color) {
  draw_quad(texture, pos, dim, 0.0f, color);
}

//
// Text 
//
void draw_text(Font font, char* text, Vec2 pos, f32 font_size, f32 spacing, Vec4 color) {
  DrawTextEx(font, text, rl_vec2(pos), font_size, spacing, rl_color(color));
}
