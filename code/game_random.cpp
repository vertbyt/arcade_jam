
//
// Game Random
//

struct Random_Series {
  u32 seed;
  u32 index;
};

void random_begin(Random_Series* series, u32 seed) {
  series->seed = seed;
  series->index = seed;
}

u32 random_u32(Random_Series* series) {
  u32 r = series->index;

  // xorshift from handmade hero
  r ^= (r << 13);
  r ^= (r >> 7);
  r ^= (r << 17);
  
  series->index = r;
  return r;
}

b32 random_b32(Random_Series* series) {
  b32 r = (random_u32(series)%2 == 0);
  return r;
}

f32 random_f32(Random_Series* series) {
  f32 d = (f32)(random_u32(series)%1000 + 1);
  f32 r = d/1000.0f;
  return r;
}


b32 random_chance(Random_Series* series, int value) {
  b32 r = (random_u32(series)%value == 0);
  return r;
}

s32 random_range(Random_Series* random, s32 min, s32 max) {
  s32 r = min + (s32)(random_u32(random)%(max - min));
  return r;
}

global_var Random_Series global_random;

void random_begin(u32 seed)  { random_begin(&global_random, seed); }
u32  random_u32(void)         { return random_u32(&global_random); }
b32  random_b32(void)         { return random_b32(&global_random); }
f32  random_f32(void)         { return random_f32(&global_random); }

b32  random_chance(int value)       { return random_chance(&global_random, value); }
s32  random_range(int min, int max) { return random_range(&global_random, min, max); }
