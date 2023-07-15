#include "game_base.cpp"
#include "game_math.cpp"
#include "game_random.cpp"
#include "game_timer.cpp"
#include "game_memory.cpp"
#include "game_asset_catalog.cpp"
#include "game_draw.cpp"

#define ASPECT_4_BY_3 1

#if ASPECT_4_BY_3
// 640×480, 800×600, 960×720, 1024×768, 1280×960
#define WINDOW_WIDTH  1024
#define WINDOW_HEIGHT 768
#else
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#endif


#define TARGET_FPS        60
#define TARGET_DELTA_TIME (1.0f/(f32)TARGET_FPS)

#define TITLE         "Arcade Jam"

// Colors
Vec4 RED_VEC4    = {1, 0, 0, 1};
Vec4 GREEN_VEC4  = {0, 1, 0, 1};
Vec4 BLUE_VEC4   = {0, 0, 1, 1};
Vec4 YELLOW_VEC4 = {1, 1, 0, 1};

Vec4 BLACK_VEC4 = {0, 0, 0, 1};
Vec4 WHITE_VEC4 = {1, 1, 1, 1};


struct Polygon {
  Vec2* points;
  s32 point_count;
};

Polygon polygon_create_explosion(s32 point_count, f32 jaggedness, f32 start_angle) {
  Allocator *allocator = get_allocator();
  
  Polygon r = {};
  r.points = allocator_alloc_array(allocator, Vec2, point_count);
  r.point_count = point_count;
  
  f32 angle = start_angle;
  f32 angle_step = (2*Pi32)/(f32)point_count;
  Vec2* points = r.points;
  Loop(i, point_count) {
    f32 scale = 1.0f - random_f32()*jaggedness;
    points[i] = vec2(angle)*scale;
    angle += angle_step;
  }
  
  return r;
}

void draw_triangle(Vec2 v0, Vec2 v1, Vec2 v2, Vec4 color) {
  DrawTriangle(rl_vec2(v0), rl_vec2(v1), rl_vec2(v2), rl_color(color));
}

void draw_polygon(Polygon polygon, Vec2 center, f32 scale, f32 rot, Vec4 color) {
  Loop(i, polygon.point_count) {
    s32 next_index = (i + 1)%polygon.point_count;

    Vec2 p1 = center + polygon.points[i]*scale;    
    Vec2 p2 = center + polygon.points[next_index]*scale;
   
    draw_triangle(p1, center, p2, color);
  }
}

void draw_polygon(Polygon poly1, Polygon poly2, f32 lerp_t, Vec2 center, f32 scale, f32 rot, Vec4 color) {
  if(poly1.point_count != poly2.point_count) {
    draw_polygon(poly1, center, scale, rot, color);
    return;
  }
  
  Polygon* a = &poly1;
  Polygon* b = &poly2;
  
  Loop(i, a->point_count) {
    s32 j = (i + 1)%a->point_count;
    
    Vec2 v1 = vec2_rotate(vec2_lerp(a->points[i], b->points[i], lerp_t), rot);
    Vec2 p1 = center + v1*scale;    
    
    Vec2 v2 = vec2_rotate(vec2_lerp(a->points[j], b->points[j], lerp_t), rot);
    Vec2 p2 = center + v2*scale;
    
    draw_triangle(p1, center, p2, color);
    
  }
}

// Defs
enum Entity_Type {
  Entity_Type_None,
  
  Entity_Type_Player,
  Entity_Type_Projectile,

  Entity_Type_Turret,
  Entity_Type_Goon,
  Entity_Type_Goon_Leader,
  
  Entity_Type_Chain_Block,
  
  Entity_Type_Count
  
};

enum Entity_State {
  Entity_State_None,
  
  Entity_State_Initial,
  Entity_State_Active,
  Entity_State_Waiting,
  Entity_State_Telegraphing,
  
  Entity_State_Count
};


struct Entity_Index {
  s32 value;
};

struct Entity {
  Entity_Index index;
  
  Vec2 pos;
  Vec2 dir;
  f32 move_speed;
  f32 rotation;
  f32 radius;
  Vec4 color;

  s32 initial_hit_points;
  s32 hit_points;

  f32 shoot_angle;
  Timer shoot_cooldown_timer;
  
  s32 blinked_count;
  Timer blink_timer;
  
  Timer health_bar_display_timer;
  
  b32 has_entered_state;
  Timer state_timer;
  Entity_State state;
  
  Entity_Type from;
  
  b32 should_remove;
  Entity_Type type;
};

void entity_set_hit_points(Entity* e, s32 ammount) {
  e->initial_hit_points = ammount;
  e->hit_points = ammount;
}

void entity_change_state(Entity* entity, Entity_State state) {
  entity->state = state;
  entity->has_entered_state = false;
}

b32 entity_enter_state(Entity* entity) {
  f32 r = !entity->has_entered_state;
  entity->has_entered_state = true;
  return r;
}

struct Chain_Circle {
  Vec2 pos;
  f32 radius;
  f32 target_radius;
  f32 emerge_time;
  f32 life_time;
  f32 life_prolong_time;
  b32 should_remove;
};



struct Score_Dot {
  Vec2 pos;
  f32 blink_time;
  b32 is_blinking;
  b32 should_remove;
};

struct Game_State {
  Entity* entities;
  s32 entity_count;

  Chain_Circle* chain_circles;
  s32 chain_circle_count;

  Score_Dot* score_dots;
  s32 score_dot_count;
  
  Polygon explosion_polygons[4];
  
  Texture2D chain_circle_texture;
  Texture2D goon_texture;
  Texture2D goon_leader_texture;
  
  Font font;
  
  s32 score;
  
  b32 is_init;
};


#define SMALL_CHAIN_CIRCLE 40.0f 
#define BIG_CHAIN_CIRCLE   100.0f

#define CHAIN_CIRCLE_EMERGE_TIME        0.03f
#define MAX_CHAIN_CIRCLE_LIFE_TIME      2.0f
#define CHAIN_CIRCLE_LIFE_PROLONG_TIME  0.4f

#define SCORE_DOT_RADIUS 6

#define MAX_ENTITIES      512
#define MAX_CHAIN_CIRCLES 512
#define MAX_SCORE_DOTS    512


global_var Game_State global_game_state;

Game_State* get_game_state(void) { return &global_game_state; }


Entity* entity_new(Entity_Type type = Entity_Type_None) {
  Game_State* gs = get_game_state();

  Assert(gs->entity_count < MAX_ENTITIES);

  Entity* r = &gs->entities[gs->entity_count];
  *r = {};
  
  r->index = {gs->entity_count};
  r->type = type;
  r->state = Entity_State_Initial;
  r->should_remove = false;

  gs->entity_count += 1;

  return r;
}

void entity_remove(Entity* entity) {
  Game_State* gs = get_game_state();
  
  Entity_Index index = entity->index;
  Entity* a = &gs->entities[index.value];
  Entity* b = &gs->entities[gs->entity_count - 1];

  *a = *b;
  a->index = index;
  
  gs->entity_count -= 1;
}


void chain_circle_spawn(Vec2 pos, f32 radius) {
  
  Game_State* gs = get_game_state();
  Assert(gs->chain_circle_count < MAX_CHAIN_CIRCLES);
  
  Chain_Circle* c = &gs->chain_circles[gs->chain_circle_count];
  *c = {};
  c->pos    = pos;
  c->target_radius = radius;

  gs->chain_circle_count += 1;
}

void chain_circle_remove(s32 index) {
  Game_State* gs = get_game_state();

  gs->chain_circles[index] = gs->chain_circles[gs->chain_circle_count - 1];
  gs->chain_circle_count -= 1;
}

void score_dot_spawn(Vec2 pos, b32 is_blinking = false) {
  Game_State* gs = get_game_state();

  Score_Dot* dot = &gs->score_dots[gs->score_dot_count];
  *dot = {pos, 0.0f, is_blinking, false};
  gs->score_dot_count += 1;
}

// NOTE: I guess I'm putting functions that use window dimension calculations here,
// since I don't want to have them all over the code.
b32 is_circle_completely_offscreen(Vec2 pos, f32 radius) {
  b32 r = (pos.x < -radius || pos.x > WINDOW_WIDTH + radius ||
           pos.y < -radius || pos.y > WINDOW_HEIGHT + radius);
  return r;
}

Vec2 random_screen_pos(f32 border_x = 0.0f, f32 border_y = 0.0f) {
  f32 x = border_x + ((f32)WINDOW_WIDTH  - 2*border_x)*random_f32();
  f32 y = border_y + ((f32)WINDOW_HEIGHT - 2*border_y)*random_f32();
  return {x, y};
}

Vec2 get_screen_center(void) {
  Vec2 r = vec2(WINDOW_WIDTH, WINDOW_HEIGHT)*0.5f;
  return r;
}

b32 check_circle_vs_circle(Vec2 p0, f32 r0, Vec2 p1, f32 r1) {
  b32 r = vec2_length(p0 - p1) <= r0 + r1;
  return r;
}

void player_update(Entity* player) {
  f32 delta_time = GetFrameTime();

  Vec2 shoot_dir = {};
  
  if(IsKeyDown(KEY_UP))    shoot_dir.y -= 1;
  if(IsKeyDown(KEY_DOWN))  shoot_dir.y += 1;
  if(IsKeyDown(KEY_LEFT))  shoot_dir.x -= 1;
  if(IsKeyDown(KEY_RIGHT)) shoot_dir.x += 1;

  timer_step(&player->shoot_cooldown_timer, delta_time);
      
  b32 want_to_shoot = vec2_length(shoot_dir) > 0.0f;
  b32 can_shoot     = !timer_is_active(player->shoot_cooldown_timer);
  if(want_to_shoot && can_shoot) {
    shoot_dir = vec2_normalize(shoot_dir);

    Entity* p = entity_new(Entity_Type_Projectile);
    p->pos = player->pos;
    p->radius = 6;
    p->color = YELLOW_VEC4;
    p->dir = shoot_dir;
    p->move_speed = 650;
    p->from = Entity_Type_Player;
    
    timer_reset(&player->shoot_cooldown_timer);
  }
      
  Vec2 move_dir = {};
  if(IsKeyDown(KEY_W)) move_dir.y -= 1;
  if(IsKeyDown(KEY_S)) move_dir.y += 1;
  if(IsKeyDown(KEY_A)) move_dir.x -= 1;
  if(IsKeyDown(KEY_D)) move_dir.x += 1;

  move_dir = vec2_normalize(move_dir);

  f32 move_speed = 300.0f;
  Vec2 vel = move_dir*move_speed;
  Vec2 move_delta = vel*delta_time;

  player->pos += move_delta;

}

void projectile_update(Entity* entity) {
  f32 delta_time = GetFrameTime();

  Vec2 vel = entity->dir*entity->move_speed;
  Vec2 move_delta = vel*delta_time;
  
  entity->pos += move_delta;
  
  if(is_circle_completely_offscreen(entity->pos, entity->radius)) {
    entity->should_remove = true;
  }
}

void turret_update(Entity* entity) {
  Game_State* gs = get_game_state();
  f32 delta_time = GetFrameTime();
  
  switch(entity->state) {
    case Entity_State_Initial: {
      entity->shoot_angle = random_f32()*2.0f*Pi32;
      entity_change_state(entity, Entity_State_Waiting);
    }break;
    case Entity_State_Waiting: {
      if(entity_enter_state(entity)) {
        entity->state_timer = timer_start(5.0f);
      }

      if(timer_step(&entity->state_timer, delta_time)) {
        entity_change_state(entity, Entity_State_Telegraphing);
      }
    }break;
    case Entity_State_Telegraphing: {
      if(entity_enter_state(entity)) {
        entity->state_timer = timer_start(1.5f);
        entity->blink_timer = timer_start(0.12f);
        entity->blinked_count = 0;
      }
      
      if(timer_step(&entity->blink_timer, delta_time)) {
        timer_reset(&entity->blink_timer);
        entity->blinked_count += 1;
        
        if(entity->blinked_count % 2 == 0) entity->color = BLUE_VEC4;
        else                               entity->color = WHITE_VEC4;
      }
      
      if(timer_step(&entity->state_timer, delta_time)) {
        entity->color = BLUE_VEC4;
        
        s32 bullet_count = 16;
        
        f32 angle  = entity->shoot_angle;
        f32 spread = Pi32;
        f32 step   = spread/(f32)bullet_count;
        
        Loop(i, bullet_count) {
          Vec2 shoot_dir = vec2(angle);

          Entity* p = entity_new(Entity_Type_Projectile);
          p->pos = entity->pos;
          p->dir = shoot_dir;
          p->radius = 8.0f;
          p->color = WHITE_VEC4;
          p->move_speed = 100.0f;
          p->from = Entity_Type_Turret;
          
          angle += step;
        }
        
        entity->shoot_angle += Pi32/6.0f;
        entity_change_state(entity, Entity_State_Waiting);
      }
    }break;
  }
  
  Loop(i, gs->entity_count) {
    Entity* p = &gs->entities[i];
    if(p->should_remove) continue;
    if(p->type != Entity_Type_Projectile) continue;
    if(p->from == Entity_Type_Turret) continue;
    
    Vec2 d = entity->pos - p->pos;
    if(vec2_length(d) < entity->radius + p->radius) {
      entity->hit_points -= 1;
      entity->health_bar_display_timer = timer_start(1.25f);
      
      p->should_remove = true;
    }
  }
  
  timer_step(&entity->health_bar_display_timer, delta_time);
  if(entity->hit_points <= 0) {
    entity->should_remove = true;
    chain_circle_spawn(entity->pos, BIG_CHAIN_CIRCLE);
  }
}

void goon_update(Entity* entity) {
  Game_State* gs = get_game_state();
  f32 delta_time = GetFrameTime();
  
  Vec2 vel = entity->dir*entity->move_speed;
  Vec2 move_delta = vel*delta_time;
  entity->pos += move_delta;
  
  switch(entity->state) {
    case Entity_State_Initial: {
      if(entity_enter_state(entity)) {
        entity->state_timer = timer_start(10.0f);
      }

      b32 on_screen = !is_circle_completely_offscreen(entity->pos, entity->radius);
      
      if(on_screen) {
        entity_change_state(entity, Entity_State_Active);
      }else {
        if(timer_step(&entity->state_timer, delta_time)) {
          entity->should_remove = true;
        }
      }
    }break;
    case Entity_State_Active: {
      Loop(i, gs->entity_count) {
        Entity* p = &gs->entities[i];
        if(p->should_remove) continue;
        if(p->type != Entity_Type_Projectile) continue;
        if(p->from != Entity_Type_Player) continue;
        
        if(check_circle_vs_circle(entity->pos, entity->radius, p->pos, p->radius)) {
          entity->hit_points -= 1;
          entity->health_bar_display_timer = timer_start(1.25f);

          p->should_remove = true;
        }
      }

      timer_step(&entity->health_bar_display_timer, delta_time);

      if(entity->hit_points <= 0) {
        entity->should_remove = true;
        chain_circle_spawn(entity->pos, SMALL_CHAIN_CIRCLE);
      }
      
      if(is_circle_completely_offscreen(entity->pos, entity->radius))
        entity->should_remove = true;
    }break;
  }
}


#define GOON_LEADER_COLOR  vec4(0xFF1B68E6)
#define GOON_COLOR         vec4(0xFFE6DC1E)
#define GOON_OUTLINE_COLOR BLACK_VEC4

#define GOON_LEADER_RADIUS 20.0f
#define GOON_RADIUS        15.0f
#define GOON_PADDING       8.0f
#define GOON_MOVE_SPEED    75.0f

void spawn_goon_formation(char* formation, s32 formation_width, s32 formation_height) {

  // Find leader position
  s32 lx = 0;
  s32 ly = 0;

  b32 found_leader = false;
  Loop(y, formation_height) {
    Loop(x, formation_width) {
      if(formation[y*formation_width + x] == '#') {
        lx = x;
        ly = y;
        found_leader = true;
        break;
      }
    }
    if(found_leader) break;
  }

  // Compute goon local positions to leader
  f32 initial_offset = GOON_LEADER_RADIUS + GOON_PADDING + GOON_RADIUS;
  f32 offset_step = 2*GOON_RADIUS + GOON_PADDING;
  
  Vec2 goon_local_positions[128];
  s32 goon_count = 0;
  
  Loop(y, formation_height) {
    Loop(x, formation_width) {
      char c = formation[y*formation_width + x];
      if(c == '.' || c == '#') continue;

      s32 dx = lx - x;
      s32 dy = ly - y;
      
      f32 gx = initial_offset + (f32)(Abs(dx) - 1)*offset_step;
      f32 gy = initial_offset + (f32)(Abs(dy) - 1)*offset_step;

      gx *= Sign(dx);
      gy *= Sign(dy);

      goon_local_positions[goon_count] = vec2(gx, gy);
      goon_count += 1;
    }
  }

  // Calc leader position and dir angle
  f32 offset_step_ammount = Max(round_f32_to_s32((f32)formation_width/2.0f),
                                round_f32_to_s32((f32)formation_height/2.0f));
  
  f32 offset_radius = initial_offset + offset_step_ammount*offset_step;
  f32 total_offset = offset_radius + offset_radius*random_f32();
  
  Vec2 leader_pos = random_screen_pos();

  s32 side = random_range(0, 4);
  if(side == 0) leader_pos.x = 0 - total_offset;
  if(side == 1) leader_pos.x = WINDOW_WIDTH + total_offset;
  if(side == 2) leader_pos.y = 0 - total_offset;
  if(side == 3) leader_pos.y = WINDOW_HEIGHT + total_offset;

  Vec2 dir = get_screen_center() - leader_pos;
  f32 one_or_neg_one = random_b32() ? 1 : -1;
  f32 angle_span = Pi32/4;
  f32 dir_angle = vec2_angle(dir) + one_or_neg_one*angle_span*0.5f;
  
  // Creating and positioning the goons.
  s32 hit_points = 2;
  
  Entity* leader = entity_new(Entity_Type_Goon_Leader);
  
  leader->pos        = leader_pos;
  leader->dir        = vec2(dir_angle);
  leader->rotation   = dir_angle;
  leader->radius     = GOON_LEADER_RADIUS;
  leader->color      = GOON_LEADER_COLOR;
  leader->move_speed = GOON_MOVE_SPEED;
  entity_set_hit_points(leader, hit_points);

  Loop(i, goon_count) {
    //f32 perp_dir_angle = dir_angle;
    Vec2 local_pos = vec2_rotate(goon_local_positions[i], dir_angle);
    
    Entity* g = entity_new(Entity_Type_Goon);
    
    g->pos        = leader->pos + local_pos;
    g->dir        = leader->dir;
    g->rotation   = dir_angle;
    g->radius     = GOON_RADIUS;
    g->color      = GOON_COLOR;
    g->move_speed = GOON_MOVE_SPEED;
    entity_set_hit_points(g, hit_points);
  }
}

void spawn_goon_column() {
  char* column = {
    "*"
    "*"
    "#"
    "*"
    "*"
  };
  spawn_goon_formation(column, 1, 5);
}

void spawn_goon_ufo() {

  char* thruster = {
    "..*.."
    ".***."
    "..#.."
    ".***."
    "..*.."
  };

  char* tank = {
    "..*.."
    "*****"
    ".*#*."
    "*****"
    "..*.."
  };

  char* ufo = {
    ".***."
    "*.*.*"
    "**#**"
    "*.*.*"
    ".***."
  };
  
  spawn_goon_formation(tank, 5, 5);
}

void game_init(void) {
  
  // seed random
  {
    u32 r0 = GetRandomValue(0, 0xFFFF);
    u32 r1 = GetRandomValue(0, 0xFFFF);
    u32 seed = r0 | (r1 << 16);
    random_begin(seed);
  };
  
  // Asset catalog
  asset_catalog_init();
  asset_catalog_add("imgs");
  asset_catalog_add("run_tree/imgs");
  asset_catalog_add("fonts");
  asset_catalog_add("run_tree/fonts");

  // Allocator
  Allocator* allocator = get_allocator();
  
  u32 allocator_size = MB(24);
  u8* allocator_base = (u8*)MemAlloc(allocator_size);
  *allocator = allocator_create(allocator_base, allocator_size);
  
  // Game state init
  Game_State* game_state = get_game_state();
  
  game_state->entities = allocator_alloc_array(allocator, Entity, MAX_ENTITIES);
  game_state->entity_count = 0;

  game_state->chain_circles = allocator_alloc_array(allocator, Chain_Circle, MAX_CHAIN_CIRCLES);
  game_state->chain_circle_count = 0;
  
  game_state->score_dots = allocator_alloc_array(allocator, Score_Dot, MAX_SCORE_DOTS);
  game_state->score_dot_count = 0;
  
  game_state->chain_circle_texture = texture_asset_load("chain_circle.png");
  
  game_state->goon_texture        = texture_asset_load("goon_ship.png");
  game_state->goon_leader_texture = texture_asset_load("goon_leader_ship.png");
  
  game_state->font = font_asset_load("karmina.ttf", 32);
  
  // explosion polygons
  Loop(i, ArrayCount(game_state->explosion_polygons)) {
    game_state->explosion_polygons[i] = polygon_create_explosion(24, 0.5f, 0.0f);
  }
  
  // init some entities
  {
    Entity* player = entity_new(Entity_Type_Player);
    player->pos = vec2(WINDOW_WIDTH, WINDOW_HEIGHT)*0.5f;
    player->radius = 10.0f;
    player->color = WHITE_VEC4;
    player->shoot_cooldown_timer = timer_start(12*TARGET_DELTA_TIME);
    entity_set_hit_points(player, 1);

  
    // Turret for testing
    Entity* turret = entity_new(Entity_Type_Turret);
    turret->pos = random_screen_pos(120, 120);
    turret->radius = 20.0f;
    turret->color = BLUE_VEC4;
    turret->hit_points = 10;
    entity_set_hit_points(turret, 10);

    spawn_goon_ufo();
  };
  
  game_state->is_init = true;

}

void game_update(void) {
  Game_State* game_state = get_game_state();
  f32 delta_time = GetFrameTime();
  
  local_persist Timer goon_timer = timer_start(1.0f);

  if(timer_step(&goon_timer, delta_time)) {
    spawn_goon_column();
    goon_timer = timer_start(1.0f + 2*random_f32());
  }

  // update chain circles
  Loop(i, game_state->chain_circle_count) {
    Chain_Circle* c = &game_state->chain_circles[i];
    if(c->should_remove) continue;

    b32 emerged = c->emerge_time > CHAIN_CIRCLE_EMERGE_TIME;
    if(!emerged) {
      f32 t = c->emerge_time/CHAIN_CIRCLE_EMERGE_TIME;
      c->radius = c->target_radius*t*t;

      c->emerge_time += delta_time;
      continue;
    }

    c->radius = c->target_radius;
    
    Loop(i, game_state->entity_count) {
      Entity* e = &game_state->entities[i];
      if(e->should_remove) continue;
      if(e->type == Entity_Type_Player) continue;

      b32 is_projectile = e->type == Entity_Type_Projectile;
      b32 got_hit = check_circle_vs_circle(c->pos, c->radius, e->pos, e->radius);

      if(got_hit) {
        e->should_remove = true;
  
        if(is_projectile) {
          c->life_prolong_time = CHAIN_CIRCLE_LIFE_PROLONG_TIME;
        }else {
          b32 got_big_radius = (e->type == Entity_Type_Turret);
          f32 radius = got_big_radius ? BIG_CHAIN_CIRCLE : SMALL_CHAIN_CIRCLE;
          chain_circle_spawn(e->pos, radius);
          score_dot_spawn(e->pos); 
        }
      }
    }
    
    f32 life_advance = delta_time;
    if(c->life_prolong_time > 0.0f) {
      c->life_prolong_time -= delta_time;
      life_advance *= 0.4f;
    }   
    
    c->life_time += life_advance;
    if(c->life_time > MAX_CHAIN_CIRCLE_LIFE_TIME) c->should_remove = true;
  }

  // update entities
  Loop(i, game_state->entity_count) {
    Entity* entity = &game_state->entities[i];

    if(entity->should_remove) continue;
    
    switch(entity->type) {
      case Entity_Type_Player:     { player_update(entity); } break;
      case Entity_Type_Projectile: { projectile_update(entity); } break;
      case Entity_Type_Turret:     { turret_update(entity); } break;
      case Entity_Type_Goon: 
      case Entity_Type_Goon_Leader: {
       goon_update(entity); 
      } break;
    }
  }


  // update score dots
  {
    Entity* player = NULL;
    Loop(i, game_state->entity_count) {
      if(game_state->entities[i].type == Entity_Type_Player) {
        player = &game_state->entities[i];
        break;
      }
    }
    
    Loop(i, game_state->score_dot_count) {
      Score_Dot* dot = &game_state->score_dots[i];
      
      if(check_circle_vs_circle(dot->pos, SCORE_DOT_RADIUS, player->pos, player->radius)) {  
        s32 value = dot->is_blinking ? 5 : 1;
        game_state->score += value;
        
        dot->should_remove = true;
      }
    }
  };
  
  // Removing stuff from arrays
  {
    // Remove score dots
    for(s32 i = 0; i < game_state->score_dot_count;) {
      Score_Dot* dot = &game_state->score_dots[i];
      if(dot->should_remove) {
        *dot = game_state->score_dots[game_state->score_dot_count - 1];
        game_state->score_dot_count -= 1;
      }else {
        i += 1;
      }
    }
    
    // Remove entities.
    for(s32 i = 0; i < game_state->entity_count;) {
      Entity* entity = &game_state->entities[i];
      if(entity->should_remove) entity_remove(entity);
      else                      i += 1;
    }  
  
    // Remove chain circles
    for(s32 i = 0; i < game_state->chain_circle_count;) {
      Chain_Circle* c = &game_state->chain_circles[i];
      if(c->should_remove) chain_circle_remove(i);
      else                 i += 1;
    }
  }  
}

void game_render(void) {
  Game_State* game_state = get_game_state();
  f32 delta_time = GetFrameTime();
  
  BeginDrawing();
  
  ClearBackground(rl_color(0.2f, 0.2f, 0.2f, 1.0f));
  
  Loop(i, game_state->entity_count) {
    Entity* entity = &game_state->entities[i];
  
    switch(entity->type) {
      case Entity_Type_Player: {
        Vec2 pos = entity->pos;
        Vec2 dim = vec2(2,2)*entity->radius; 
        f32 thickness = 3;
        draw_quad        (pos - dim*0.5f, dim, vec4_fade_alpha(WHITE_VEC4, 0.45f));
        draw_quad_outline(pos - dim*0.5f, dim, thickness, WHITE_VEC4);
      }break;
      case Entity_Type_Goon: 
      case Entity_Type_Goon_Leader: {
        Vec2 dim = vec2(1, 1)*entity->radius*2;
        f32 thickness = 3.0f;
        
        f32 scale = 1.0f - thickness/entity->radius;
        draw_quad(entity->pos - dim*0.5f, dim, entity->rotation, GOON_OUTLINE_COLOR);        
        draw_quad(entity->pos - dim*0.5f*scale, dim*scale, entity->rotation, entity->color);
        
        b32 show_health = timer_is_active(entity->health_bar_display_timer);
        if(show_health) {
          Vec2 top_left = entity->pos - dim*0.5f;
          
          f32 hp_bar_h   = 7;
          f32 hp_bar_pad = 4;
          
          f32 life = (f32)entity->hit_points/(f32)entity->initial_hit_points;
          f32 hp_bar_w = dim.width*life;
          
          Vec2 hp_bar_dim = vec2(hp_bar_w, hp_bar_h);
          Vec2 hp_bar_pos = top_left - vec2(0, hp_bar_h + hp_bar_pad);
          
          draw_quad(hp_bar_pos, hp_bar_dim, RED_VEC4);
        }
      }break;
      case Entity_Type_Turret: 
      case Entity_Type_Projectile: {
        Vec2 dim = vec2(1, 1)*entity->radius*2;
        draw_quad(entity->pos - dim*0.5f, dim, entity->rotation, entity->color);
        
        b32 show_health = timer_is_active(entity->health_bar_display_timer);
        if(show_health) {
          Vec2 top_left = entity->pos - dim*0.5f;
          
          f32 hp_bar_h   = 7;
          f32 hp_bar_pad = 4;
          
          f32 life = (f32)entity->hit_points/(f32)entity->initial_hit_points;
          f32 hp_bar_w = dim.width*life;
          
          Vec2 hp_bar_dim = vec2(hp_bar_w, hp_bar_h);
          Vec2 hp_bar_pos = top_left - vec2(0, hp_bar_h + hp_bar_pad);
          
          draw_quad(hp_bar_pos, hp_bar_dim, RED_VEC4);
        }
      }break;
    }
  }
  
  Loop(i, game_state->chain_circle_count){
    Chain_Circle* c = &game_state->chain_circles[i];
    Vec2 dim = vec2(1,1)*2*c->radius;
    Vec2 pos = c->pos - dim*0.5f;
  
    Vec4 color = WHITE_VEC4;
    if(c->life_prolong_time > 0.0f) color = YELLOW_VEC4;
    draw_quad(game_state->chain_circle_texture, pos, dim, color);
  
    f32 t = Max(c->life_time,0.0f)/MAX_CHAIN_CIRCLE_LIFE_TIME;
  
    t = -t*t + 2*t; // Ease out quadratic
  
    Vec2 indicator_dim = dim*t*0.7f;
    Vec2 indicator_pos = c->pos - indicator_dim*0.5f;
    draw_quad(game_state->chain_circle_texture, indicator_pos, indicator_dim, color);
  }
  
  Vec2 dot_dim = vec2(1,1)*2*SCORE_DOT_RADIUS;
  Loop(i, game_state->score_dot_count) {
    Score_Dot* dot = &game_state->score_dots[i];
    
    Vec4 color = WHITE_VEC4;
    if(dot->is_blinking) color = YELLOW_VEC4;
    draw_quad(dot->pos - dot_dim*0.5f, dot_dim, color);  
  }
  
  s32 ep_count = ArrayCount(game_state->explosion_polygons);
  local_persist s32 ep_index = 0;
  local_persist f32 flicker_time = 0.0f;
  f32 flicker_end_time = 0.075f;
  
  local_persist f32 rot = 0.0f;
  // rot += delta_time;
  
  flicker_time += delta_time;
  if(flicker_time > flicker_end_time){
    ep_index = (ep_index + 1)%ep_count;
    flicker_time = 0;
  }
  
  Polygon a = game_state->explosion_polygons[ep_index];
  Polygon b = game_state->explosion_polygons[(ep_index + 1)%ep_count];
  f32 lerp_t = flicker_time/flicker_end_time;
  
  Vec4 inner_color  = vec4(0xFFFA971D);
  Vec4 outter_color = vec4(0xFFFBCF12);
  Vec2 mp = vec2(GetMouseX(), GetMouseY());
  
  draw_polygon(a, b, lerp_t, mp, 52, rot, BLACK_VEC4);
  draw_polygon(a, b, lerp_t, mp, 50, rot, outter_color);
  draw_polygon(a, b, lerp_t, mp, 40,  rot, inner_color);
  
  
  draw_polygon(a, b, lerp_t, mp + vec2(100, 0), 50, 0.5f, outter_color);
  draw_polygon(a, b, lerp_t, mp + vec2(100, 0), 40, 0.5f, inner_color);
  
  char* score_text = (char*)TextFormat("Score: %d\n", game_state->score);
  draw_text(game_state->font, score_text, {5, 5}, 32, 0, WHITE_VEC4);
  EndDrawing();
}

void game_update_and_render(void) {
  game_update();
  game_render();
}