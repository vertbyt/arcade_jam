#include "game_base.cpp"
#include "game_math.cpp"
#include "game_memory.cpp"

#include "game_asset_catalog.cpp"
#include "game_timer.cpp"
#include "game_random.cpp"

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

// Utils
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


// Polygon
struct Polygon {
  Vec2* points;
  s32 point_count;
};

Polygon polygon_alloc(s32 point_count) {
  Polygon r = {};
  
  Allocator* allocator = get_allocator();
  r.points = allocator_alloc_array(allocator, Vec2, point_count);
  r.point_count = point_count;
  
  return r;
}


Polygon polygon_create(s32 point_count, f32 jaggedness, f32 start_angle) {
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


void polygon_lerp(Polygon a, Polygon b, f32 lerp_t, Polygon* out) {
  Assert(a.point_count == b.point_count);
  Assert(a.point_count == out->point_count);
  
  Loop(i, a.point_count) {
    out->points[i] = vec2_lerp(a.points[i], b.points[i], lerp_t);
  }
}

void draw_polygon(Polygon polygon, Vec2 center, f32 scale, f32 rot, Vec4 color) {
  Loop(i, polygon.point_count) {
    s32 next_index = (i + 1)%polygon.point_count;

    Vec2 p1 = center + vec2_rotate(polygon.points[i], rot)*scale;    
    Vec2 p2 = center + vec2_rotate(polygon.points[next_index], rot)*scale;
   
    draw_triangle(p1, center, p2, color);
  }
}

// Defs
enum Entity_Type {
  Entity_Type_None,
  
  Entity_Type_Player,
  Entity_Type_Turret,
  Entity_Type_Goon,
  Entity_Type_Chain_Activator,
  
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


struct Entity_Id {
  s64 value;
};

struct Entity_Index {
  s32 value;
};

struct Entity_Base {  
  Entity_Index index;
  Entity_Id    id;
  Entity_Type  type;
  
  Vec2 pos;
  Vec2 dir;
  f32 move_speed;
  f32 rotation;
  f32 scale;
  f32 radius;
  Vec4 color;

  s32 initial_hit_points;
  s32 hit_points;
  Timer health_bar_display_timer;

  b32 has_entered_state;
  Timer state_timer;
  Entity_State state;
  
  b32 should_remove;
};

struct Player : public Entity_Base {
  f32 shoot_angle;
  Timer shoot_cooldown_timer;
};

struct Turret : public Entity_Base {
  f32 shoot_angle;
  s32 blinked_count;
  Timer blink_timer;
};

struct Chain_Activator : public Entity_Base {
  Vec2 target;
  f32 orbitals_rot;
  
  struct {
    Vec2 positions[3];
    f32  rotations[3];
  } orbitals;
};

struct Goon : public Entity_Base {
  s32 no_data;
};

struct Entity {
  union {
    Entity_Base     base;
    Player          player;
    Turret          turret;
    Chain_Activator chain_activator;
  };
  
  Entity_Type type;
};


void entity_set_hit_points(Entity_Base* base, s32 ammount) {
  base->initial_hit_points = ammount;
  base->hit_points = ammount;
}

void entity_change_state(Entity_Base* base, Entity_State state) {
  base->state = state;
  base->has_entered_state = false;
}

b32 entity_enter_state(Entity_Base* base) {
  f32 r = !base->has_entered_state;
  base->has_entered_state = true;
  return r;
}

struct Projectile {
  Vec2 pos, vel, dir;
  f32 rotation;
  f32 move_speed;
  f32 radius;
  
  Vec4 color;
  
  Entity_Type from_type;
  Entity_Id   from_id;

  b32 is_active;
};

struct Chain_Circle {
  Vec2 pos;
  f32 radius;
  f32 target_radius;
  f32 emerge_time;
  f32 life_time;
  f32 life_prolong_time;
  
  b32 is_active;
};

struct Explosion {
  Vec2 pos;
  f32 scale, rot;
  Timer timer;
  
  b32 is_active;
};

struct Score_Dot {
  Vec2 pos;
  f32 blink_time;
  b32 is_blinking;
  
  b32 is_active;
};

struct Game_State {
  // State
  Entity* entities;
  s32 entity_count;
  s32 next_entity_id;
  
  Projectile* projectiles;
  s32 next_projectile_index;
  s32 active_projectile_count;
  
  Chain_Circle* chain_circles;
  s32 next_chain_circle_index;
  s32 active_chain_circle_count;

  Score_Dot* score_dots;
  s32 next_score_dot_index;
  s32 active_score_dot_count;

  Explosion* explosions;
  s32 next_explosion_index;
  s32 active_explosion_count;
  
  s32 score;
  b32 is_init;
    
  // Explosion polygon instance
  Polygon explosion_polygons[8];
  s32 explosion_polygon_index;
  Timer explosion_timer;
  Polygon current_explosion_frame_polygon;
  
  // Assets
  Texture2D chain_circle_texture;
  Texture2D chain_activator_texture;
  Font font;
  
  // Perf
  b32 show_debug_info;
  f64 update_time;
  f64 draw_time;
};


#define SMALL_CHAIN_CIRCLE 40.0f 
#define BIG_CHAIN_CIRCLE   100.0f

#define CHAIN_CIRCLE_EMERGE_TIME        0.03f
#define MAX_CHAIN_CIRCLE_LIFE_TIME      2.0f
#define CHAIN_CIRCLE_LIFE_PROLONG_TIME  0.4f

#define SCORE_DOT_RADIUS 6

#define MAX_ENTITIES      128

#define MAX_PROJECTILES   256
#define MAX_CHAIN_CIRCLES 256
#define MAX_SCORE_DOTS    256

#define MAX_EXPLOSIONS    8


global_var Game_State global_game_state;

Game_State* get_game_state(void) { return &global_game_state; }

Entity* new_entity(Entity_Type type = Entity_Type_None) {
  Game_State* gs = get_game_state();

  Assert(gs->entity_count < MAX_ENTITIES);

  Entity* entity = &gs->entities[gs->entity_count];
  *entity = {};
  
  entity->type = type;
  
  Entity_Base* base = &entity->base;
  base->index = {gs->entity_count};
  base->id    = {gs->next_entity_id};
  base->type  = type;
  base->state = Entity_State_Initial;

  gs->entity_count   += 1;
  gs->next_entity_id += 1;

  return entity;
}

void remove_entity(Entity_Base* base) { base->should_remove = true; }
void remove_entity(Entity*  entity)   { entity->base.should_remove = true; }

void actually_remove_entities(void) {
  Game_State* gs = get_game_state();
  
  Loop(i, gs->entity_count) {
    Entity* curr = &gs->entities[i];
    if(curr->base.should_remove) {
      Entity* last = &gs->entities[gs->entity_count - 1];
      *curr = *last;
      
      gs->entity_count -= 1;
    }
  }
}

Projectile* new_projectile() {
  Game_State* gs = get_game_state();

  Projectile* p = &gs->projectiles[gs->next_projectile_index];
  
  *p = {};
  p->is_active = true;
  
  gs->next_projectile_index += 1;
  gs->next_projectile_index %= MAX_PROJECTILES;
  
  return p;
}

void remove_projectile(Projectile* p) { p->is_active = false; }

void spawn_chain_circle(Vec2 pos, f32 radius) {
  Game_State* gs = get_game_state();
  
  Chain_Circle* c = &gs->chain_circles[gs->next_chain_circle_index];
  *c = {};
  c->pos           = pos;
  c->target_radius = radius;
  c->is_active     = true;
  
  gs->next_chain_circle_index += 1;
  gs->next_chain_circle_index %= MAX_CHAIN_CIRCLES;
}

void remove_chain_circle(Chain_Circle* c) { c->is_active = false; } 


void spawn_score_dot(Vec2 pos, b32 is_blinking = false) {
  Game_State* gs = get_game_state();

  Score_Dot* dot = &gs->score_dots[gs->next_score_dot_index];
  *dot = {pos, 0.0f, is_blinking, true};
  
  gs->next_score_dot_index += 1;
  gs->next_score_dot_index %= MAX_SCORE_DOTS;
}

void remove_score_dot(Score_Dot* dot) { dot->is_active = false;}

void spawn_explosion(Vec2 pos, f32 scale, f32 time) {
  Game_State* gs = get_game_state();

  Explosion* e = &gs->explosions[gs->next_explosion_index];
  e->pos = pos;
  e->scale = scale;
  e->rot = 2.0f*Pi32*random_f32();
  e->timer = timer_start(time);
  e->is_active = true;
  
  gs->next_explosion_index += 1;
  gs->next_explosion_index %= MAX_EXPLOSIONS;
}

void remove_explosion(Explosion* e) { e->is_active = false; }


//
// @update
//
void update_player(Entity* entity) {
  Player* player = (Player*)entity;
  
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

    Projectile* p = new_projectile();
    p->pos = player->pos;
    p->radius = 6;
    p->color = YELLOW_VEC4;
    p->dir = shoot_dir;
    p->move_speed = 650;
    p->from_type = Entity_Type_Player;
    p->from_id = player->id;
    
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


void update_turret(Entity* entity) {
  Game_State* gs = get_game_state();
  f32 delta_time = GetFrameTime();
  
  Turret* turret = &entity->turret;
  
  switch(turret->state) {
    case Entity_State_Initial: {
      turret->shoot_angle = random_f32()*2.0f*Pi32;
      entity_change_state(turret, Entity_State_Waiting);
    }break;
    case Entity_State_Waiting: {
      if(entity_enter_state(turret)) {
        turret->state_timer = timer_start(5.0f);
      }

      if(timer_step(&turret->state_timer, delta_time)) {
        entity_change_state(turret, Entity_State_Telegraphing);
      }
    }break;
    case Entity_State_Telegraphing: {
      if(entity_enter_state(turret)) {
        turret->state_timer = timer_start(1.5f);
        turret->blink_timer = timer_start(0.12f);
        turret->blinked_count = 0;
      }
      
      if(timer_step(&turret->blink_timer, delta_time)) {
        timer_reset(&turret->blink_timer);
        turret->blinked_count += 1;
        
        if(turret->blinked_count % 2 == 0) turret->color = BLUE_VEC4;
        else                               turret->color = WHITE_VEC4;
      }
      
      if(timer_step(&turret->state_timer, delta_time)) {
        turret->color = BLUE_VEC4;
        
        s32 bullet_count = 16;
        
        f32 angle  = turret->shoot_angle;
        f32 spread = Pi32;
        f32 step   = spread/(f32)bullet_count;
        
        Loop(i, bullet_count) {
          Vec2 shoot_dir = vec2(angle);

          Projectile* p = new_projectile();
          p->pos = turret->pos;
          p->dir = shoot_dir;
          p->radius = 8.0f;
          p->color = WHITE_VEC4;
          p->move_speed = 100.0f;
          p->from_type = Entity_Type_Turret;
          p->from_id   = turret->id;
          
          angle += step;
        }
        
        turret->shoot_angle += Pi32/6.0f;
        entity_change_state(turret, Entity_State_Waiting);
      }
    }break;
  }
  
  
  Loop(i, MAX_PROJECTILES) {
    Projectile* p = &gs->projectiles[i];
    if(!p->is_active) continue;
    if(p->from_type != Entity_Type_Player) continue;
    
    Vec2 d = turret->pos - p->pos;
    if(vec2_length(d) < turret->radius + p->radius) {
      turret->hit_points -= 1;
      turret->health_bar_display_timer = timer_start(1.25f);
      
      remove_projectile(p);
    }
  }
  
  
  timer_step(&turret->health_bar_display_timer, delta_time);
  if(turret->hit_points <= 0) {
    remove_entity(turret);
    spawn_chain_circle(turret->pos, BIG_CHAIN_CIRCLE);
  }
}


void update_goon(Entity* entity) {
  Game_State* gs = get_game_state();
  f32 delta_time = GetFrameTime();
  
  Entity_Base* goon = &entity->base;
  
  Vec2 vel = goon->dir*goon->move_speed;
  Vec2 move_delta = vel*delta_time;
  goon->pos += move_delta;
  
  switch(goon->state) {
    case Entity_State_Initial: {
      if(entity_enter_state(goon)) {
        goon->state_timer = timer_start(10.0f);
      }

      b32 on_screen = !is_circle_completely_offscreen(goon->pos, goon->radius);
      
      if(on_screen) {
        entity_change_state(goon, Entity_State_Active);
      }else {
        if(timer_step(&goon->state_timer, delta_time)) remove_entity(goon);
      }
    }break;
    case Entity_State_Active: {
      Loop(i, MAX_PROJECTILES) {
        Projectile* p = &gs->projectiles[i];
        if(!p->is_active) continue;
        if(p->from_type != Entity_Type_Player) continue;
        
        if(check_circle_vs_circle(goon->pos, goon->radius, p->pos, p->radius)) {
          goon->hit_points -= 1;
          goon->health_bar_display_timer = timer_start(1.25f);

          remove_projectile(p);
        }
      }

      timer_step(&goon->health_bar_display_timer, delta_time);

      if(goon->hit_points <= 0) {
        remove_entity(goon);
        //spawn_explosion(goon->pos, SMALL_CHAIN_CIRCLE, 1.0f);
        spawn_chain_circle(goon->pos, SMALL_CHAIN_CIRCLE);
      }
      
      if(is_circle_completely_offscreen(goon->pos, goon->radius)) remove_entity(goon);
    }break;
  }
}


void update_projectiles(void) {
  Game_State* gs = get_game_state();
  f32 delta_time = GetFrameTime();
  
  Loop(i, MAX_PROJECTILES) {
    Projectile* p = &gs->projectiles[i];
    if(!p->is_active) continue;
    
    Vec2 vel = p->dir*p->move_speed;
    Vec2 move_delta = vel*delta_time;
    
    p->pos += move_delta;
    
    if(is_circle_completely_offscreen(p->pos, p->radius)) remove_projectile(p);
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
  
  Goon* leader = (Goon*)new_entity(Entity_Type_Goon);
  
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
    
    Goon* g = (Goon*)new_entity(Entity_Type_Goon);
    
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
void update_explosion_polygon() {
  Game_State* gs = get_game_state();
  f32 delta_time = GetFrameTime();

  s32 ep_count = ArrayCount(gs->explosion_polygons);
  
  if(timer_step(&gs->explosion_timer, delta_time)) {
    gs->explosion_polygon_index += 1;
    gs->explosion_polygon_index %= ep_count;
    timer_reset(&gs->explosion_timer);
  }  

  Polygon* frame = &gs->current_explosion_frame_polygon;
  Polygon a = gs->explosion_polygons[gs->explosion_polygon_index];
  Polygon b = gs->explosion_polygons[(gs->explosion_polygon_index + 1)%ep_count];
  f32 t = timer_procent(gs->explosion_timer);
  polygon_lerp(a, b, t, frame);
}

void draw_explosion_polygon(Vec2 pos, f32 scale, f32 rot) {
  Game_State* gs = get_game_state();
  
  Polygon poly = gs->current_explosion_frame_polygon;
  
  Vec4 inner_color  = vec4(0xFFFA971D);
  Vec4 outter_color = vec4(0xFFFBCF12);
  
  draw_polygon(poly, pos, scale,      rot, BLACK_VEC4);
  draw_polygon(poly, pos, scale - 6,  rot, outter_color);
  draw_polygon(poly, pos, scale*0.5f, rot, inner_color);
}

void update_explosions() {
  Game_State* gs = get_game_state();
  f32 delta_time = GetFrameTime();
  
  Loop(i, MAX_EXPLOSIONS) {
    Explosion* e = &gs->explosions[i];
    if(!e->is_active) continue;
    
    if(timer_step(&e->timer, delta_time)) e->is_active = false;
  }
}

void update_chain_circles() {
  Game_State* gs = get_game_state();
  f32 delta_time = GetFrameTime();
  
  // update chain circles
  Loop(i, MAX_CHAIN_CIRCLES) {
    Chain_Circle* c = &gs->chain_circles[i];
    if(!c->is_active) continue;

    b32 emerged = c->emerge_time > CHAIN_CIRCLE_EMERGE_TIME;
    if(!emerged) {
      f32 t = c->emerge_time/CHAIN_CIRCLE_EMERGE_TIME;
      c->radius = c->target_radius*t*t;

      c->emerge_time += delta_time;
      continue;
    }

    c->radius = c->target_radius;
    
    Loop(i, MAX_PROJECTILES) {
      Projectile* p = &gs->projectiles[i];
      if(!p->is_active) continue;
      
      if(check_circle_vs_circle(c->pos, c->radius, p->pos, p->radius)) {
        c->life_prolong_time = CHAIN_CIRCLE_LIFE_PROLONG_TIME;
        remove_projectile(p);
        break;
      }
    }
    
    Loop(i, gs->entity_count) {
      Entity_Base* e = (Entity_Base*)&gs->entities[i];
      if(e->type == Entity_Type_Player) continue;
      
      if(check_circle_vs_circle(c->pos, c->radius, e->pos, e->radius)) {
        b32 got_big_radius = (e->type == Entity_Type_Turret);
        f32 radius = got_big_radius ? BIG_CHAIN_CIRCLE : SMALL_CHAIN_CIRCLE;
        spawn_chain_circle(e->pos, radius);
        spawn_score_dot(e->pos); 
        
        remove_entity(e);
      }
    }
    
    f32 life_advance = delta_time;
    if(c->life_prolong_time > 0.0f) {
      c->life_prolong_time -= delta_time;
      life_advance *= 0.4f;
    }   
    
    c->life_time += life_advance;
    if(c->life_time > MAX_CHAIN_CIRCLE_LIFE_TIME) remove_chain_circle(c);
  }
}

void update_score_dots() {
  Game_State* gs = get_game_state();
  
  Player* player = NULL;
  Loop(i, gs->entity_count) {
    if(gs->entities[i].type == Entity_Type_Player) {
      player = (Player*)&gs->entities[i];
      break;
    }
  }
  
  Loop(i, MAX_SCORE_DOTS) {
    Score_Dot* dot = &gs->score_dots[i];
    if(!dot->is_active) continue;
    
    if(check_circle_vs_circle(dot->pos, SCORE_DOT_RADIUS, player->pos, player->radius)) {  
      s32 value = dot->is_blinking ? 5 : 1;
      gs->score += value;
      remove_score_dot(dot);
    }
  }
}


void update_entities(void) {
  Game_State* gs = get_game_state();
  
  // update entities
  Loop(i, gs->entity_count) {
    Entity* entity = &gs->entities[i];
    switch(entity->type) {
      case Entity_Type_Player:     { update_player(entity); } break;
      case Entity_Type_Turret:     { update_turret(entity); } break;
      case Entity_Type_Goon:       { update_goon(entity); } break;
    }
  }
}

void count_active_game_objects(void) {
  Game_State* gs = get_game_state();
    
  gs->active_projectile_count   = 0;
  gs->active_chain_circle_count = 0;
  gs->active_score_dot_count    = 0;
  gs->active_explosion_count    = 0;
  
  Loop(i, MAX_PROJECTILES)   
    gs->active_projectile_count += gs->projectiles[i].is_active;
  
  Loop(i, MAX_CHAIN_CIRCLES) 
    gs->active_chain_circle_count += gs->chain_circles[i].is_active;
  
  Loop(i, MAX_SCORE_DOTS) 
    gs->active_score_dot_count += gs->score_dots[i].is_active;
    
  Loop(i, MAX_EXPLOSIONS)
     gs->active_explosion_count += gs->explosions[i].is_active;
}

void update_game(void) {
  Game_State* gs = get_game_state();
  f32 delta_time = GetFrameTime();
  
  f64 start_time = GetTime();
  
  local_persist Timer goon_timer = timer_start(1.0f);

  if(timer_step(&goon_timer, delta_time)) {
    spawn_goon_column();
    goon_timer = timer_start(1.0f + 2*random_f32());
  }

  update_entities();  
  actually_remove_entities();

  update_projectiles();
  update_explosion_polygon();
  update_explosions();
  update_chain_circles();
  update_score_dots();
  
  count_active_game_objects();
  
  f64 end_time = GetTime();
  gs->update_time = end_time - start_time;
}

//
// @draw
//
void draw_debug_info(void)  {
  Game_State* gs = get_game_state();
  
  if(IsKeyPressed(KEY_Q)) gs->show_debug_info = !gs->show_debug_info;
  if(!gs->show_debug_info) return;
  
  Vec2 pos = {10, 10};
  f32 font_size = 24;
  
  draw_text(gs->font, "Debug Info:", pos, font_size, 0, WHITE_VEC4);
  pos.y += font_size;
  
  char* score_text = (char*)TextFormat("entity_count: %d\n", gs->entity_count);
  draw_text(gs->font, score_text, pos, font_size, 0, WHITE_VEC4);
  pos.y += font_size;
  
  score_text = (char*)TextFormat("projectile_count: %d\n", gs->active_projectile_count);
  draw_text(gs->font, score_text, pos, font_size, 0, WHITE_VEC4);
  pos.y += font_size;
  
  
  score_text = (char*)TextFormat("chain_circle_count: %d\n", gs->active_chain_circle_count);
  draw_text(gs->font, score_text, pos, font_size, 0, WHITE_VEC4);
  pos.y += font_size;
  
  
  score_text = (char*)TextFormat("score_dot_count: %d\n", gs->active_score_dot_count);
  draw_text(gs->font, score_text, pos, font_size, 0, WHITE_VEC4);
  pos.y += font_size;
  
  
  score_text = (char*)TextFormat("explosion_count: %d\n", gs->active_explosion_count);
  draw_text(gs->font, score_text, pos, font_size, 0, WHITE_VEC4);
  pos.y += font_size;
  
  
  f64 update_ms  = gs->update_time*1000.0f;
  s32 update_fps = (s32)(1.0f/gs->update_time);
  score_text = (char*)TextFormat("update_ms:  %.4f[%d fps]\n", update_ms, update_fps);
  draw_text(gs->font, score_text, pos, font_size, 0, WHITE_VEC4);
  pos.y += font_size;
  
  
  f64 draw_ms  = gs->draw_time*1000.0f;
  s32 draw_fps = (s32)(1.0f/gs->draw_time);
  score_text = (char*)TextFormat("draw_ms:      %.4f[%d fps]\n", draw_ms, draw_fps);
  draw_text(gs->font, score_text, pos, font_size, 0, WHITE_VEC4);
  pos.y += font_size;
  
  
  f64 frame_ms  = GetFrameTime();
  s32 frame_fps = (s32)(1.0f/GetFrameTime());
  score_text = (char*)TextFormat("frame_ms:     %.4f[%d fps]\n", frame_ms, frame_fps);
  draw_text(gs->font, score_text, pos, font_size, 0, WHITE_VEC4);
  pos.y += font_size;
}

void draw_entities(void) {
  Game_State* game_state = get_game_state();

  Loop(i, game_state->entity_count) {
    Entity* the_entity = &game_state->entities[i];
    Entity_Base* entity = (Entity_Base*)the_entity;
  
    switch(entity->type) {
      case Entity_Type_Player: {
        Vec2 pos = entity->pos;
        Vec2 dim = vec2(2,2)*entity->radius; 
        f32 thickness = 3;
        draw_quad        (pos - dim*0.5f, dim, vec4_fade_alpha(WHITE_VEC4, 0.45f));
        draw_quad_outline(pos - dim*0.5f, dim, thickness, WHITE_VEC4);
      }break;
      case Entity_Type_Goon: {
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
      case Entity_Type_Turret: {
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
}

void draw_projectiles(void) {
  Game_State* gs = get_game_state();
  
  Loop(i, MAX_PROJECTILES) {
    Projectile* p = &gs->projectiles[i];
    if(!p->is_active) continue;
    Vec2 dim = vec2(1, 1)*p->radius*2;
    draw_quad(p->pos - dim*0.5f, dim, p->rotation, p->color);
  }
}


void draw_chain_circles(void) {
  Game_State* game_state = get_game_state();

  Loop(i, MAX_CHAIN_CIRCLES){
    Chain_Circle* c = &game_state->chain_circles[i];
    if(!c->is_active) continue;
    
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
  
}

void draw_score_dots(void) {
  Game_State* gs = get_game_state();
  
  Vec2 dot_dim = vec2(1,1)*2*SCORE_DOT_RADIUS;
  Loop(i, MAX_SCORE_DOTS) {
    Score_Dot* dot = &gs->score_dots[i];
    if(!dot->is_active) continue;
    
    Vec4 color = WHITE_VEC4;
    if(dot->is_blinking) color = YELLOW_VEC4;
    draw_quad(dot->pos - dot_dim*0.5f, dot_dim, color);  
  }
}


void draw_explosions() {
  Game_State* gs = get_game_state();
  
  Loop(i, MAX_EXPLOSIONS) {
    Explosion* e = &gs->explosions[i];
    if(!e->is_active) continue;
    
    draw_explosion_polygon(e->pos, e->scale, e->rot);    
  }
}

void draw_game(void) {
  Game_State* gs = get_game_state();
  
  BeginDrawing();
  ClearBackground(rl_color(0.2f, 0.2f, 0.2f, 1.0f));
  
  f64 start_time = GetTime();
  
  draw_entities();
  
  draw_projectiles();
  draw_chain_circles();
  draw_score_dots();
  draw_explosions();
  
  f64 end_time = GetTime();
  gs->draw_time = end_time - start_time;
  draw_debug_info();
  
  char* score_text = (char*)TextFormat("Score: %d\n", gs->score);
  draw_text(gs->font, score_text, {450, 5}, 24, 0, WHITE_VEC4);
  
  EndDrawing();
}


//
//@init
//
void init_game(void) {
  
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

  game_state->projectiles   = allocator_alloc_array(allocator, Projectile,   MAX_PROJECTILES);
  game_state->chain_circles = allocator_alloc_array(allocator, Chain_Circle, MAX_CHAIN_CIRCLES);
  game_state->explosions    = allocator_alloc_array(allocator, Explosion,    MAX_EXPLOSIONS);
  game_state->score_dots    = allocator_alloc_array(allocator, Score_Dot,    MAX_SCORE_DOTS);
  
  
  game_state->chain_circle_texture    = texture_asset_load("chain_circle.png");
  game_state->chain_activator_texture = texture_asset_load("chain_activator.png");
   
  game_state->font = font_asset_load("karmina.ttf", 24);
  
  // explosion polygon
  s32 point_count = 18;
  game_state->explosion_polygon_index = 0;
  game_state->explosion_timer = timer_start(0.12f);
  game_state->current_explosion_frame_polygon = polygon_alloc(point_count);
  Loop(i, ArrayCount(game_state->explosion_polygons)) {
    game_state->explosion_polygons[i] = polygon_create(point_count, 0.5f, 0.0f);
  }
  
  // init some entities
  {
    Player* player = (Player*)new_entity(Entity_Type_Player);
    player->pos = vec2(WINDOW_WIDTH, WINDOW_HEIGHT)*0.5f;
    player->radius = 10.0f;
    player->color = WHITE_VEC4;
    player->shoot_cooldown_timer = timer_start(12*TARGET_DELTA_TIME);
    entity_set_hit_points(player, 1);

  
    // Turret for testing
    Turret* turret = (Turret*)new_entity(Entity_Type_Turret);
    turret->pos = random_screen_pos(120, 120);
    turret->radius = 20.0f;
    turret->color = BLUE_VEC4;
    turret->hit_points = 10;
    entity_set_hit_points(turret, 10);

    spawn_goon_ufo();
  };
  
  game_state->is_init = true;
}

void do_game_loop(void) {
  update_game();
  draw_game();
}
