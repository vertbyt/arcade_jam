#include "game_base.cpp"
#include "game_math.cpp"
#include "game_memory.cpp"

#include "game_asset_catalog.cpp"
#include "game_timer.cpp"
#include "game_random.cpp"

#include "game_draw.cpp"

#include "game_tweek.cpp"


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

f32 random_angle(void) { return(2.0f*Pi32*random_f32()); }

Vec2 random_offscreen_pos(f32 offset) {
 Vec2 r = random_screen_pos();

  s32 side = random_range(0, 4);
  if(side == 0) r.x = 0 - offset;
  if(side == 1) r.x = WINDOW_WIDTH + offset;
  if(side == 2) r.y = 0 - offset;
  if(side == 3) r.y = WINDOW_HEIGHT + offset;
  
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
  Entity_Type_None = 0,
  
  Entity_Type_Player            = (1 << 0),  
  Entity_Type_Goon              = (1 << 2),
  Entity_Type_Laser_Turret      = (1 << 3),
  Entity_Type_Triple_Gun_Turret = (1 << 4),
  Entity_Type_Chain_Activator   = (1 << 5),
  Entity_Type_Infector          = (1 << 6),
};

enum Entity_State {
  Entity_State_None,
  
  Entity_State_Initial,
  Entity_State_Offscreen,
  Entity_State_Emerge,
  Entity_State_Active,
  Entity_State_Targeting,
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
  Vec2 vel;
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
  
  b32 is_active;
};

struct Player : public Entity_Base {
  f32 flap;
  
  f32 wobble;
  f32 wobble_scale;
  Timer wobble_timer;
  
  f32 shoot_indicator;
  Timer shoot_indicator_timer;
  
  f32 score_sound_delay_time;
  
  f32 target_turn_angle;
  f32 turn_angle;
  f32 shoot_angle;
  Timer shoot_cooldown_timer;
};

struct Laser_Turret : public Entity_Base {
  f32 shoot_angle;
  s32 blinked_count;
  Timer blink_timer;
};

struct Triple_Gun_Turret : public Entity_Base {
  s32 projectiles_left_to_spawn;
  Timer projectile_spawn_timer;
};

struct Chain_Activator : public Entity_Base {
  Vec4 start_color, end_color;
  f32 start_radius, end_radius;
  
  f32 orbital_radius;
  f32 orbital_global_rotation;
    
  b32 for_tutorial_purposes;
  char* text_line;
  
  struct {
    f32 rotation;
    f32 time;
    f32 active;
  } orbitals[5];
};

struct Goon : public Entity_Base {
  s32 no_data;
};

struct Infector : public Entity_Base {
  f32 wobble;
};

struct Entity {
  union {
    Entity_Base       base;
    Player            player;
    Laser_Turret      laser_turret;
    Triple_Gun_Turret triple_gun_turret; 
    Chain_Activator   chain_activator;
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
  
  b32 has_life_time;
  Timer life_timer;
  
  Timer emit_timer;
    
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
  
  b32 is_infected;
  Timer infection_timer;
  f32 infection;
  
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
  b32 is_special;
  f32 life_time;
  
  f32 pulse_time;    
  f32 pulse_radius;
  
  b32 is_active;
};


struct Particle {
  Vec2 pos, vel;
  f32 friction;
  f32 radius;
  f32 rotation;
  Timer life_timer;
  Vec4 color;
  
  b32 is_active;
};


enum Game_Screen {
  Game_Screen_Menu,
  Game_Screen_Credits,
  Game_Screen_Game,
  Game_Screen_Paused,
  Game_Screen_Death,
  Game_Screen_Win,
};

struct High_Score {
  s32 score;
  s32 lives;
};

struct Game_State {
  // game controls
  Timer show_game_controls_timer;
  
  s32 level_played_times;  
    
  // game screen
  Game_Screen game_screen;
  b32 has_entered_game_screen;
  s32 option_index;
  s32 master_volume;
  
  // game objects
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
  
  Particle* particles;
  s32 next_particle_index;
  s32 active_particles_count;
  
  // level state
  f32 level_duration;
  f32 level_time_passed;
  s32 score;
  High_Score high_score;
  b32 got_high_score;
  
  struct {
    Timer goon, laser_turret, triple_turret, activator, infector;
  } spawn_timer;
  b32 are_spawn_timers_init;
  
  // explosion polygon instance
  Polygon explosion_polygons[8];
  s32 explosion_polygon_index;
  Timer explosion_timer;
  Polygon current_explosion_frame_polygon;
  
  // Butterfly 
  Vec2 butterfly_top_wing[5];  
  Vec2 butterfly_bottom_wing[5];
  
  // assets
  Texture2D chain_circle_texture;
  Texture2D chain_activator_texture;
  Texture2D laser_bullet_texture;
  Font small_font, medium_font, big_font;
  
  Sound player_shoot_sound;
  Sound explosion_sound;
  Sound laser_shot_sound;
  Sound score_pickup_sound;
  Sound player_hit_sound;
  
  Music songs[2];
  s32 song_index;
  Timer song_timer;
  b32 is_level_music_done;

  // perf
  b32 show_debug_info;
  f64 update_time;
  f64 draw_time;
};


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
  base->is_active = true;
  
  gs->entity_count   += 1;
  gs->next_entity_id += 1;

  return entity;
}

void remove_entity(Entity_Base* base) { base->is_active = false; }
void remove_entity(Entity*  entity)   { entity->base.is_active = false; }

void actually_remove_entities(void) {
  Game_State* gs = get_game_state();
  
  Loop(i, gs->entity_count) {
    Entity* curr = &gs->entities[i];
    if(!curr->base.is_active) {
      Entity* last = &gs->entities[gs->entity_count - 1];
      *curr = *last;
      
      gs->entity_count -= 1;
    }
  }
}


Particle* new_particle() {
  Game_State* gs = get_game_state();

  Particle* p = &gs->particles[gs->next_particle_index];
  
  *p = {};
  p->is_active = true;
  
  gs->next_particle_index += 1;
  gs->next_particle_index %= MAX_PARTICLES;
  
  return p;
}

void remove_particle(Particle* p) { p->is_active = false; }


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

void projectile_set_parent(Projectile* p, Entity* entity) {
  p->from_type = entity->type;
  p->from_id = entity->base.id;
}

void projectile_set_life_time(Projectile* p, f32 life_time) {
  p->has_life_time = true;
  p->life_timer = timer_start(life_time);
}

Chain_Circle* spawn_chain_circle(Vec2 pos, f32 radius) {
  Game_State* gs = get_game_state();
  
  Chain_Circle* c = &gs->chain_circles[gs->next_chain_circle_index];
  *c = {};
  c->pos           = pos;
  c->target_radius = radius;
  c->is_active     = true;
  
  gs->next_chain_circle_index += 1;
  gs->next_chain_circle_index %= MAX_CHAIN_CIRCLES;
  
  return c;
}


void infect_chain_circle(Chain_Circle* c) {
  if(!c->is_infected) {
    c->is_infected = true;
    c->infection = 0.0f;
    c->infection_timer = timer_start(CHAIN_CIRCLE_INFECTION_TIME);
  }
}

void spawn_infected_chain_circle(Vec2 pos, f32 radius) {
  Chain_Circle* c = spawn_chain_circle(pos, radius);
  infect_chain_circle(c);
}

void remove_chain_circle(Chain_Circle* c) { c->is_active = false; } 


void spawn_score_dot(Vec2 pos, b32 is_special = false) {
  Game_State* gs = get_game_state();

  Score_Dot* dot = &gs->score_dots[gs->next_score_dot_index];
  *dot = {};
  
  dot->pos = pos;
  dot->is_special = is_special;
  dot->is_active = true;
    
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
  
  PlaySound(gs->explosion_sound);
}

void remove_explosion(Explosion* e) { e->is_active = false; }


#define PARTICLE_TRAIL_VELOCITY_RANGE     {50, 100}
#define PARTICLE_TRAIL_FRICTION_RANGE     {0.95, 0.99}
#define PARTICLE_TRAIL_RADIUS_RANGE       {1, 3}
#define PARTICLE_TRAIL_LIFE_RANGE         {0.05f, 0.08f}
#define PARTICLE_TRAIL_ANGLE_LEEWAY_RANGE {-(Pi32/4), (Pi32/4)}

void spawn_particle_trial(Vec2 pos, Vec2 dir, s32 count, Vec4 color) {
  Loop(i, count) {
    Particle* p = new_particle();
    f32 rot = vec2_angle(dir) + vec2_lerp_x_to_y(PARTICLE_TRAIL_ANGLE_LEEWAY_RANGE, random_f32());
    
    p->pos        = pos;
    p->vel        = vec2(rot)*vec2_lerp_x_to_y(PARTICLE_TRAIL_VELOCITY_RANGE, random_f32());
    p->friction   = vec2_lerp_x_to_y(PARTICLE_TRAIL_FRICTION_RANGE, random_f32());
    p->radius     = vec2_lerp_x_to_y(PARTICLE_TRAIL_RADIUS_RANGE, random_f32());
    p->life_timer = timer_start(vec2_lerp_x_to_y(PARTICLE_TRAIL_LIFE_RANGE, random_f32()));
    p->rotation   = rot;
    p->color      = color;
  }
}

Player* get_player(void) {
  Game_State* gs = get_game_state();
  Player* r = NULL;
  Loop(i, gs->entity_count) {
    if(gs->entities[i].type == Entity_Type_Player) {
      r = (Player*)&gs->entities[i];
      break;
    }
  }
  return r;
}

// collision checks agains groups of game objects
b32 check_collision_vs_chain_circles(Vec2 pos, f32 radius) {
  b32 hit = false;
  
  Game_State* gs = get_game_state();
  Loop(i, MAX_CHAIN_CIRCLES) {
    Chain_Circle* c = &gs->chain_circles[i];
    if(!c->is_active) continue;
      
    if(check_circle_vs_circle(pos, radius, c->pos, c->radius)) { hit = true; break; }
  }
  
  return hit;
}

//
// @update
//
Vec2 player_process_input_rhs(void) {
  Vec2 dir = {};
  
  if(IsKeyDown(KEY_UP))    dir.y -= 1;
  if(IsKeyDown(KEY_DOWN))  dir.y += 1;
  if(IsKeyDown(KEY_LEFT))  dir.x -= 1;
  if(IsKeyDown(KEY_RIGHT)) dir.x += 1;
  
  if(IsGamepadAvailable(0)) {
  
    Vec2 dpad = {};
    if(IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_UP))    dpad.y -= 1;
    if(IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))  dpad.y += 1;
    if(IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT))  dpad.x -= 1;
    if(IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) dpad.x += 1;
    
    if(vec2_length(dpad) > 0.0f) dir = dpad;
    
    Vec2 stick = {GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X),
                  GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y)};
    
    if(vec2_length(stick) > 0.0f) dir = stick;
  }
  
  return dir;
}

Vec2 player_process_input_lhs(void) {
  Vec2 dir = {};
  
  if(IsKeyDown(KEY_W)) dir.y -= 1;
  if(IsKeyDown(KEY_S)) dir.y += 1;
  if(IsKeyDown(KEY_A)) dir.x -= 1;
  if(IsKeyDown(KEY_D)) dir.x += 1;
  
  if(IsGamepadAvailable(0)) {
  
    Vec2 dpad = {};
    if(IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_UP))    dpad.y -= 1;
    if(IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN))  dpad.y += 1;
    if(IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT))  dpad.x -= 1;
    if(IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) dpad.x += 1;
    
    if(vec2_length(dpad) > 0.0f) dir = dpad;
    
    Vec2 stick = {GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X),
                  GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y)};
    
    if(vec2_length(stick) > 0.0f) dir = stick;
  }
  
  return dir;
}

void update_player(Entity* entity) {
  Game_State* gs = get_game_state();
  Player* player = (Player*)entity;
  f32 delta_time = GetFrameTime();
  
  if(player->hit_points <= 0) return;
  
  player->score_sound_delay_time += delta_time;
  Loop(i, MAX_SCORE_DOTS) {
    Score_Dot* dot = &gs->score_dots[i];
    if(!dot->is_active) continue;
    
    f32 bigger_radius = player->radius*2.0f;
    if(check_circle_vs_circle(dot->pos, SCORE_DOT_RADIUS, player->pos, bigger_radius)) {  
      s32 value = dot->is_special ? 5 : 1;
      gs->score += value;
      remove_score_dot(dot);
      
      if(player->score_sound_delay_time > 0.075f) {
        PlaySound(gs->score_pickup_sound);    
        player->score_sound_delay_time = 0.0f;
      }
    }
  }
  
  timer_step(&player->wobble_timer, delta_time);  
  b32 is_wobbling = timer_is_active(player->wobble_timer);
  if(!is_wobbling) {
    b32 got_hit = false;
    Loop(i, gs->entity_count) {
      Entity_Base* e = (Entity_Base*)&gs->entities[i];
      if(!e->is_active) continue;
      if(e->type == Entity_Type_Player) continue;
      
      // So that we don't get killed by emerging turrest from which we don't have a chance
      // to evade
      if(e->state == Entity_State_Initial) continue;
      if(e->state == Entity_State_Emerge) continue;
      
      if(check_circle_vs_circle(player->pos, player->radius, e->pos, e->radius)) {
        got_hit = true;
        break;
      }
    }
    
    Loop(i, MAX_PROJECTILES) {
      Projectile* p = &gs->projectiles[i];
      if(!p->is_active) continue;
      if(p->from_type == Entity_Type_Player) continue;
      
      if(check_circle_vs_circle(player->pos, player->radius, p->pos, p->radius)) {
        got_hit = true;
        break;
      }
    }
    
    Loop(i, MAX_CHAIN_CIRCLES) {
      Chain_Circle* c = &gs->chain_circles[i];
      if(!c->is_active) continue;
      if(!c->is_infected) continue;
      
      if(check_circle_vs_circle(player->pos, player->radius, c->pos, c->radius*c->infection)) {
        got_hit = true;
      }
    }
    
    if(got_hit) {
      PlaySound(gs->player_hit_sound);
      player->wobble_timer = timer_start(3.0f);
      is_wobbling = true;
      player->hit_points -= 1;
    }
  }
  
  Vec2 (*process_shoot_input) (void);
  Vec2 (*process_move_input)  (void);
  
  process_shoot_input = player_process_input_rhs;
  process_move_input  = player_process_input_lhs;
  
  Vec2 shoot_dir = process_shoot_input();
  Vec2 move_dir  = process_move_input();

  shoot_dir = vec2_normalize(shoot_dir);
  move_dir = vec2_normalize(move_dir);
  
  // Shoot
  timer_step(&player->shoot_cooldown_timer, delta_time);
      
  b32 want_to_shoot = vec2_length(shoot_dir) > 0.0f;
  b32 can_shoot     = !timer_is_active(player->shoot_cooldown_timer);
  if(want_to_shoot && can_shoot) {

    Projectile* p = new_projectile();
    p->pos = player->pos;
    p->radius = 6;
    p->color = WHITE_VEC4;
    p->dir = shoot_dir;
    p->rotation = vec2_angle(shoot_dir);
    p->move_speed = 650;
    p->emit_timer = timer_start(0.0f);
    projectile_set_parent(p, (Entity*)player);
    
    timer_reset(&player->shoot_cooldown_timer);
    player->shoot_indicator_timer = timer_start(0.25f);
    
    PlaySound(gs->player_shoot_sound);
  }

  // Expand a bit when shooting
  b32 is_shoot_indicator_active = timer_is_active(player->shoot_indicator_timer);
  if(timer_step(&player->shoot_indicator_timer, delta_time)) {
    player->shoot_indicator = 0.0f;
  }
  else {
    player->flap = 0.0f;
    f32 x = timer_procent(player->shoot_indicator_timer)*2*Pi32;
    player->shoot_indicator = (cosf(x + Pi32) + 1.0f)/2.0f; 
  }
  
  // Wobble
  if(is_wobbling) {
    player->flap = 0.0f;
    
    f32 t = timer_procent(player->wobble_timer);
    ease_out_quad(&t);
    
    f32 x = 2*Pi32*t;
    player->wobble_scale = 20.0f;
    player->wobble = ((cosf(x*5.0f + Pi32) + 1)/2);
  }
  
  // Flaping
  if(!is_wobbling && !is_shoot_indicator_active) {
    player->flap = cosf(GetTime()*18.0f)*0.075f;
  }
  
  // Turning
  if(move_dir.x == 0)   player->target_turn_angle = 0.0f;
  if(move_dir.x > 0.0f) player->target_turn_angle = PLAYER_MAX_TURN_ANGLE;
  if(move_dir.x < 0.0f) player->target_turn_angle = -PLAYER_MAX_TURN_ANGLE;

  f32 turn_dir = Sign(player->target_turn_angle - player->turn_angle);
  f32 turn_delta = turn_dir*PLAYER_TURN_SPEED*delta_time;
  player->turn_angle += turn_delta;
  
  // Move
  Vec2 vel = move_dir*PLAYER_MOVE_SPEED;
  Vec2 move_delta = vel*delta_time;

  player->pos += move_delta;
  
  if(player->pos.x < 0)             player->pos.x = 0.0f;
  if(player->pos.x > WINDOW_WIDTH)  player->pos.x = WINDOW_WIDTH;
  if(player->pos.y < 0)             player->pos.y = 0.0;
  if(player->pos.y > WINDOW_HEIGHT) player->pos.y = WINDOW_HEIGHT;
}


void update_laser_turret(Entity* entity) {
  Game_State* gs = get_game_state();
  f32 delta_time = GetFrameTime();
  
  Player* player = get_player();
  Laser_Turret* turret = (Laser_Turret*)entity;

  turret->rotation = turret->shoot_angle;
  timer_step(&turret->health_bar_display_timer, delta_time);
    
  // projectile interaction  
  Loop(i, MAX_PROJECTILES) {
    Projectile* p = &gs->projectiles[i];
    if(!p->is_active) continue;
    if(p->from_type != Entity_Type_Player) continue;
    
    Vec2 d = turret->pos - p->pos;
    if(vec2_length(d) < turret->radius + p->radius) {
      remove_projectile(p);
      
      turret->hit_points -= 1;      
      if(turret->hit_points <= 0) {
        spawn_explosion(turret->pos, turret->radius*2.5f, 1.0f);
        remove_entity(turret);
        return;
      }
      
      turret->health_bar_display_timer = timer_start(1.25f);
    }
  }
  
  // chain circle interaction
  if(check_collision_vs_chain_circles(turret->pos, turret->radius)) {
    PlaySound(gs->explosion_sound);
    spawn_chain_circle(turret->pos, BIG_CHAIN_CIRCLE);
    spawn_score_dot(turret->pos, false);
    remove_entity(turret);
    return;
  }
  
  // FSM  
  switch(turret->state) {
    case Entity_State_Initial: {
      turret->pos = random_screen_pos(120, 120);
      turret->radius = 0.0f;

      turret->color = BLUE_VEC4;
      entity_set_hit_points(turret, LASER_TURRET_HIT_POINTS);
      
      turret->shoot_angle = random_f32()*2.0f*Pi32;
      entity_change_state(turret, Entity_State_Emerge);
    }break;
    case Entity_State_Emerge: {
      if(entity_enter_state(turret)) turret->state_timer = timer_start(2.0f);
      
      f32 t = timer_procent(turret->state_timer);
      t = lerp_f32(0.2f, 1.0f, t);
      ease_out_quad(&t);
      
      turret->radius = LASER_TURRET_RADIUS*t;
      
      if(timer_step(&turret->state_timer, delta_time)) {
        turret->radius = LASER_TURRET_RADIUS;
        entity_change_state(turret, Entity_State_Targeting);
      }
    }break;
    case Entity_State_Targeting: {
      if(entity_enter_state(turret)) {
        turret->state_timer = timer_start(5.0f);
      }
      
      Vec2 v = player->pos - turret->pos;
      f32 target_angle = vec2_angle(v);
      f32 lerp_speed = 0.025f;
      f32 lerp_t = lerp_speed*delta_time;
      
      f32 disp = target_angle - turret->shoot_angle;
      if(Abs(target_angle - turret->shoot_angle) >= Pi32) disp = -(2*Pi32 - disp);
      
      turret->shoot_angle += disp*lerp_speed;
            
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
        
        Vec2 shoot_dir = vec2(turret->shoot_angle);
        f32 shoot_max_len = vec2_length({WINDOW_WIDTH, WINDOW_HEIGHT});
        
        s32 bullet_count = 65;
        f32 bullet_radius = LASER_TURRET_PROJECTILE_RADIUS;
        
        f32 offset_to_gun = turret->radius + LASER_TURRET_GUN_HEIGHT + bullet_radius/2;
        Vec2 pos = turret->pos + shoot_dir*offset_to_gun;
        f32 step = shoot_max_len/bullet_count;

        Loop(i, bullet_count) {
          Projectile* p = new_projectile();
          p->pos = pos + vec2(random_angle())*random_f32()*3.0f;
          p->rotation = turret->rotation + random_f32(-1,1)*Pi32*0.2f;
          p->radius = bullet_radius;
          
          p->dir = vec2(p->rotation);
          p->move_speed = 5.0f;
          
          projectile_set_parent(p, (Entity*)turret);
          projectile_set_life_time(p, LASER_TURRET_PROJECTILE_LIFETIME);
          
          pos += shoot_dir*step;
        }
        
        entity_change_state(turret, Entity_State_Targeting);
        
        PlaySound(gs->laser_shot_sound);
      }
    }break;
  }
}

void update_triple_gun_turret(Entity* entity) {
  Game_State* gs = get_game_state();
  f32 delta_time = GetFrameTime();

  Triple_Gun_Turret* turret = (Triple_Gun_Turret*)entity;

  timer_step(&turret->health_bar_display_timer, delta_time);

  // projectile interaction 
  Loop(i, MAX_PROJECTILES) {
    Projectile* p = &gs->projectiles[i];
    if(!p->is_active) continue;
    if(p->from_type != Entity_Type_Player) continue;
    
    if(check_circle_vs_circle(turret->pos, turret->radius, p->pos, p->radius)) {
      remove_projectile(p);
      
      turret->hit_points -= 1;
      
      if(turret->hit_points <= 0) { 
        spawn_explosion(turret->pos, turret->radius*2.5f, 1.0f);
        remove_entity(turret);
        return;
      }
      
      turret->health_bar_display_timer = timer_start(1.25);
      break;
    }
  }
  
  // chain circle interaction
  if(check_collision_vs_chain_circles(turret->pos, turret->radius)) {
    PlaySound(gs->explosion_sound);
    spawn_chain_circle(turret->pos, BIG_CHAIN_CIRCLE);
    spawn_score_dot(turret->pos, false);
    remove_entity(turret);
    return;
  }
    
  // FSM
  switch(turret->state) {
    case Entity_State_Initial: {
      turret->pos = random_screen_pos(120, 120);
      turret->radius = 0.0f;
      turret->rotation = random_angle();
      turret->color = TRIPLE_GUN_TURRET_COLOR;
      entity_set_hit_points(turret, LASER_TURRET_HIT_POINTS);
      
      entity_change_state(turret, Entity_State_Emerge);
    }break;
    case Entity_State_Emerge: {
      if(entity_enter_state(turret)) turret->state_timer = timer_start(2.0f);
      
      f32 t = timer_procent(turret->state_timer);
      t = lerp_f32(0.2f, 1.0f, t);
      ease_out_quad(&t);
      
      turret->radius = TRIPLE_GUN_TURRET_RADIUS*t;
      
      if(timer_step(&turret->state_timer, delta_time)) {
        turret->radius = TRIPLE_GUN_TURRET_RADIUS;
        entity_change_state(turret, Entity_State_Waiting);
      }
    }break;
    case Entity_State_Waiting: {
      if(entity_enter_state(turret)) {
        turret->state_timer = timer_start(3.0f);
      }
      
      if(timer_step(&turret->state_timer, delta_time))
        entity_change_state(turret, Entity_State_Telegraphing);
    }break;
    case Entity_State_Telegraphing: {
      if(entity_enter_state(turret)) {
        turret->state_timer = timer_start(2.0f);
      }
      
      f32 x = 2.0f*Pi32*timer_procent(turret->state_timer);
      f32 t = (cosf(x*10 + Pi32) + 1)/2;
      turret->color = vec4_lerp(TRIPLE_GUN_TURRET_COLOR, WHITE_VEC4, t);
      
      if(timer_step(&turret->state_timer, delta_time)) {
        entity_change_state(turret, Entity_State_Active);
      }
    }break;
    case Entity_State_Active: {
      if(entity_enter_state(turret)) {
        turret->projectiles_left_to_spawn = TRIPLE_GUN_TURRET_BULLET_COUNT;
        turret->projectile_spawn_timer = timer_start(TRIPLE_GUN_TURRET_FIRE_RATE);
      }
      
      b32 should_spawn = false;      
      if(timer_step(&turret->projectile_spawn_timer, delta_time)) {
        should_spawn = true;
        timer_reset(&turret->projectile_spawn_timer);
      }      
      
      if(should_spawn) {
        f32 angle_step = TRIPLE_GUN_TURRET_GUN_ANGLE_STEP;
        f32 angle = turret->rotation - angle_step;
        
        Loop(i, 3) {
          Vec2 dir = vec2(angle);
          Vec2 pos = turret->pos + dir*(turret->radius + TRIPLE_GUN_TURRET_BULLET_RADIUS);
          
          Projectile* p = new_projectile();
          p->pos = pos;
          p->dir = dir;
          p->rotation = angle;
          p->move_speed = TRIPLE_GUN_TURRET_BULLET_MOVE_SPEED;
          p->radius = TRIPLE_GUN_TURRET_BULLET_RADIUS;
          p->color = YELLOW_VEC4;
          projectile_set_parent(p, (Entity*)turret);
          
          angle += angle_step;
        }
        
        turret->projectiles_left_to_spawn -= 1;
      }
      
      if(turret->projectiles_left_to_spawn <= 0) {
        Player* player = get_player();

        f32 rotation_dir = -1;
        f32 angle_to_player = vec2_angle(player->pos - turret->pos);
        if(Abs(angle_to_player - turret->rotation) > Pi32) rotation_dir = 1;
        
        turret->rotation = angle_to_player;//rotation_dir*(Pi32/4);
        entity_change_state(turret, Entity_State_Waiting);
      }
    }break;
  }
}


void update_goon(Entity* entity) {
  Game_State* gs = get_game_state();
  f32 delta_time = GetFrameTime();
  
  Goon* goon = (Goon*)entity;
  
  // Move
  Vec2 vel = goon->dir*goon->move_speed;
  Vec2 move_delta = vel*delta_time;
  goon->pos += move_delta;
  
  // FSM
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
      if(is_circle_completely_offscreen(goon->pos, goon->radius)) {
        remove_entity(goon);
        return;
      }
    
      // projectile interaction
      Loop(i, MAX_PROJECTILES) {
        Projectile* p = &gs->projectiles[i];
        if(!p->is_active) continue;
        if(p->from_type != Entity_Type_Player) continue;
        
        if(check_circle_vs_circle(goon->pos, goon->radius, p->pos, p->radius)) {
          goon->hit_points -= 1;
          goon->health_bar_display_timer = timer_start(1.25f);

          remove_projectile(p);
          
          if(goon->hit_points <= 0) {
            remove_entity(goon);
            spawn_explosion(goon->pos, SMALL_CHAIN_CIRCLE, 1.0f);
            return;
          }
        }
      }
      
      // chain circle interaction      
      if(check_collision_vs_chain_circles(goon->pos, goon->radius)) {
        PlaySound(gs->explosion_sound);
        spawn_chain_circle(goon->pos, SMALL_CHAIN_CIRCLE);
        spawn_score_dot(goon->pos, false);
        remove_entity(goon);
        return;
      }
      
      timer_step(&goon->health_bar_display_timer, delta_time);
      
    }break;
  }
}

void update_chain_activator(Entity* entity) {
  Game_State* gs = get_game_state();
  f32 delta_time = GetFrameTime();
  
  Chain_Activator* activator = (Chain_Activator*)entity;
  
  // FSM
  switch(activator->state) {
    case Entity_State_Initial: {
      if(!activator->for_tutorial_purposes) {
        activator->pos = random_offscreen_pos(CHAIN_ACTIVATOR_START_RADIUS*4);
        f32 angle_to_center = vec2_angle(get_screen_center() - activator->pos);
        f32 dir_angle = angle_to_center + random_f32(-1, 1)*(Pi32/6);
        
        activator->dir = vec2(dir_angle);
      }
      
      activator->move_speed = CHAIN_ACTIVATOR_MOVE_SPEED;
      
      activator->start_radius = CHAIN_ACTIVATOR_START_RADIUS;
      activator->end_radius   = CHAIN_ACTIVATOR_END_RADIUS;
      activator->start_color  = CHAIN_ACTIVATOR_START_COLOR;
      activator->end_color    = CHAIN_ACTIVATOR_END_COLOR;   
      
      activator->orbital_radius = CHAIN_ACTIVATOR_ORBITAL_RADIUS;
      
      activator->orbital_global_rotation = random_angle();
      
      Loop(i, ArrayCount(activator->orbitals)) {
        activator->orbitals[i].rotation = random_angle();
        activator->orbitals[i].active = true;
        activator->orbitals[i].time = 0.0f;
      }
    
      activator->color  = activator->start_color;
      activator->radius = activator->start_radius;
            
      entity_change_state(activator, Entity_State_Offscreen);
    }break;
    case Entity_State_Offscreen: {
      if(entity_enter_state(activator)) {
        activator->state_timer = timer_start(50.0f);
      }
      
      activator->vel = activator->move_speed*activator->dir;
      Vec2 move_delta = activator->vel*delta_time;
      activator->pos += move_delta;
      
      b32 on_screen = !is_circle_completely_offscreen(activator->pos, activator->radius);
      if(on_screen) {
        entity_change_state(activator, Entity_State_Active);
        break;
      }
      
      if(timer_step(&activator->state_timer, delta_time)) {
        remove_entity(activator);
      }      
    }break;
    case Entity_State_Active: {
      Loop(i, MAX_PROJECTILES) {
        Projectile* p = &gs->projectiles[i];
        if(!p->is_active) continue;
        if(p->from_type != Entity_Type_Player) continue;
        
        if(check_circle_vs_circle(activator->pos, activator->radius, p->pos, p->radius)) {
          remove_projectile(p);
          entity_change_state(activator, Entity_State_Telegraphing);
          break;
        }
      }
      
      if(check_collision_vs_chain_circles(activator->pos, activator->radius)) {
        entity_change_state(activator, Entity_State_Telegraphing);
      }
      
      activator->vel = activator->move_speed*activator->dir;
      Vec2 move_delta = activator->vel*delta_time;
      activator->pos += move_delta;
    }break;
    case Entity_State_Telegraphing: {
      Loop(i, MAX_PROJECTILES) {
        Projectile* p = &gs->projectiles[i];
        if(!p->is_active) continue;
        if(p->from_type != Entity_Type_Player) continue;
        
        if(check_circle_vs_circle(activator->pos, activator->radius, p->pos, p->radius)) {
          activator->vel = p->dir*350.0f;
          remove_projectile(p);
          break;
        }
      }
      
      f32 friction = 0.97f;
      activator->vel *= friction;
      Vec2 move_delta = activator->vel*delta_time;
      activator->pos += move_delta;
      
      s32 orbital_count = ArrayCount(activator->orbitals);

      if(entity_enter_state(activator)) {
        f32 telegraph_time = 2.25f;
        Loop(i, orbital_count) {
          f32 t = (f32)i/(f32)orbital_count;
          activator->orbitals[i].time = telegraph_time*t;
        }
        
        activator->state_timer = timer_start(telegraph_time);
      }
      
      activator->rotation -= 4.0f*delta_time;
      Loop(i, orbital_count) {
        activator->orbitals[i].time -= delta_time;
        activator->orbitals[i].active = activator->orbitals[i].time > 0.0f;
      }
      
      f32 lerp_t = timer_procent(activator->state_timer);
      ease_out_quad(&lerp_t);
      
      activator->radius = lerp_f32(activator->start_radius, activator->end_radius, lerp_t);
      activator->color  = vec4_lerp(activator->start_color, activator->end_color, lerp_t);

      if(timer_step(&activator->state_timer, delta_time)) {
        PlaySound(gs->explosion_sound);

        spawn_chain_circle(activator->pos, MEDIUM_CHAIN_CIRCLE);        
        remove_entity(activator);
      }
    }break;
  }  
  
  // rotation
  activator->rotation -= delta_time;
  activator->orbital_global_rotation += 1.5f*delta_time;
  
  Loop(i, ArrayCount(activator->orbitals)) {
    activator->orbitals[i].rotation -= delta_time;
  }
}

void update_infector(Entity* entity) {
  Game_State* gs = get_game_state();
  f32 delta_time = GetFrameTime();
  
  Infector* infector = (Infector*)entity;
  
  
  timer_step(&infector->health_bar_display_timer, delta_time);
  
  // projectile interaction 
  Loop(i, MAX_PROJECTILES) {
    Projectile* p = &gs->projectiles[i];
    if(!p->is_active) continue;
    if(p->from_type != Entity_Type_Player) continue;
    
    if(check_circle_vs_circle(infector->pos, infector->radius, p->pos, p->radius)) {
      remove_projectile(p);
      
      infector->hit_points -= 1;
      
      if(infector->hit_points <= 0) { 
        spawn_explosion(infector->pos, infector->radius*2.5f, 1.0f);
        remove_entity(infector);
        return;
      }
      
      infector->health_bar_display_timer = timer_start(1.25);
      break;
    }
  }
  
  // chain circle interaction
  if(check_collision_vs_chain_circles(infector->pos, infector->radius)) {
    PlaySound(gs->explosion_sound);
    spawn_infected_chain_circle(infector->pos, 80.0f);
    spawn_score_dot(infector->pos, false);
    remove_entity(infector);
    return;
  }
   
   
  // FSM
  switch(infector->state) {
    case Entity_State_Initial: {
      infector->pos = random_offscreen_pos(INFECTOR_RADIUS*4);
      
      f32 angle_to_center = vec2_angle(get_screen_center() - infector->pos);
      f32 dir_angle = angle_to_center + random_f32(-1, 1)*(Pi32/6);
      infector->dir = vec2(dir_angle);
      
      infector->radius = INFECTOR_RADIUS;
      infector->move_speed = INFECTOR_MOVE_SPEED;
      entity_set_hit_points(infector, INFECTOR_HIT_POITNS);
      
      entity_change_state(infector, Entity_State_Offscreen);
    }break;
    case Entity_State_Offscreen: {
      if(entity_enter_state(infector)) infector->state_timer = timer_start(10.0f);
      
      // Move
      Vec2 move_delta = infector->dir*infector->move_speed*delta_time;
      infector->pos += move_delta;
      
      b32 on_screen = !is_circle_completely_offscreen(infector->pos, infector->radius);
      if(on_screen) entity_change_state(infector, Entity_State_Waiting);
      
      if(timer_step(&infector->state_timer, delta_time)) remove_entity(infector);
    }break;
    case Entity_State_Waiting: {
      if(entity_enter_state(infector)) {
        infector->state_timer = timer_start(6.0f);
      }

      // Move
      Vec2 move_delta = infector->dir*infector->move_speed*delta_time;
      infector->pos += move_delta;
      
      if(timer_step(&infector->state_timer, delta_time)) {
        entity_change_state(infector, Entity_State_Telegraphing);
      }
    }break;
    case Entity_State_Telegraphing: {
      if(entity_enter_state(infector)) {
        infector->state_timer = timer_start(2.0f);
      }
      
      f32 x = 2*Pi32*timer_procent(infector->state_timer);
      f32 t = (cosf(x*8.0f + Pi32) + 1)/2.0f;
      infector->wobble = t;
      
      if(timer_step(&infector->state_timer, delta_time)) {
        f32 bullet_count = 6;
        f32 angle_step = (2*Pi32)/bullet_count;
        f32 angle = 0.0f;
        Loop(i, bullet_count) {
          Projectile* p = new_projectile();
          p->pos = infector->pos + vec2(angle)*infector->radius*0.5f;
          p->radius = 8.0f;
          p->move_speed = 200.0f;
          p->dir = vec2(angle);
          p->rotation = angle;
          p->color = RED_VEC4;
          projectile_set_parent(p, (Entity*)infector);
          
          angle += angle_step;
        }
        
        entity_change_state(infector, Entity_State_Waiting);
      }
    }break;
  }
}

void update_particles(void) {
  Game_State* gs = get_game_state();
  f32 delta_time = GetFrameTime();
  
  Loop(i, MAX_PARTICLES) {
    Particle* p = &gs->particles[i];
    if(!p->is_active) continue;
    
    p->vel *= p->friction;
    p->pos += p->vel*delta_time;
    
    if(timer_step(&p->life_timer, delta_time)) remove_particle(p);
  }
}

void draw_particles(void) {
  Game_State* gs = get_game_state();
  f32 delta_time = GetFrameTime();
  
  Loop(i, MAX_PARTICLES) {
    Particle* p = &gs->particles[i];
    if(!p->is_active) continue;
    
    Vec2 dim = vec2(2,2)*p->radius;
    draw_quad(p->pos - dim*0.5f, dim, p->rotation, p->color);
  }
}

void update_projectiles(void) {
  Game_State* gs = get_game_state();
  f32 delta_time = GetFrameTime();
  
  Loop(i, MAX_PROJECTILES) {
    Projectile* p = &gs->projectiles[i];
    if(!p->is_active) continue;
  
                           
    b32 got_hit = false;
    Chain_Circle* hit_circle = NULL;
    Loop(i, MAX_CHAIN_CIRCLES) {
      Chain_Circle* c = &gs->chain_circles[i];
      if(!c->is_active) continue;
      
      if(check_circle_vs_circle(p->pos, p->radius, c->pos, c->radius)) {
        got_hit = true;
        hit_circle = c;    
        break;
      }
    }
  
    if(got_hit) {
      b32 is_chain_bullet = (p->from_type == Entity_Type_Laser_Turret ||
                             p->from_type == Entity_Type_Triple_Gun_Turret);  
                           
      if(is_chain_bullet) {
        spawn_chain_circle(p->pos, 25.0f);
        spawn_score_dot(p->pos, true);
        remove_projectile(p);
        continue;
      }
      else if(p->from_type == Entity_Type_Infector) {
        infect_chain_circle(hit_circle);
        remove_projectile(p);
        continue;
      }
    }
        
    if(p->from_type == Entity_Type_Player) {
      if(timer_step(&p->emit_timer, delta_time)) {
        spawn_particle_trial(p->pos, -p->dir, 8, p->color);
        timer_reset(&p->emit_timer);
      }
    }
    
    Vec2 vel = p->dir*p->move_speed;
    Vec2 move_delta = vel*delta_time;
    p->pos += move_delta;
    
    if(p->has_life_time && timer_step(&p->life_timer, delta_time)) {
      remove_projectile(p);
      continue;
    }
    
    if(is_circle_completely_offscreen(p->pos, p->radius)) remove_projectile(p);
  }
}


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

    f32 lerp_speed = 10.0f;
    f32 t = lerp_speed*delta_time;
    c->radius = lerp_f32(c->radius, c->target_radius, t);
    
    Loop(i, MAX_PROJECTILES) {
      Projectile* p = &gs->projectiles[i];
      if(!p->is_active) continue;
      if(p->from_type != Entity_Type_Player) continue;
      
      if(check_circle_vs_circle(c->pos, c->radius, p->pos, p->radius)) {
        c->life_prolong_time = CHAIN_CIRCLE_LIFE_PROLONG_TIME;
        c->target_radius += 3.0f;
        remove_projectile(p);
        break;
      }
    }
    
    if(c->is_infected) {
      timer_step(&c->infection_timer, delta_time);
      c->infection = timer_procent(c->infection_timer);
      
      if(c->infection == 1.0f) {
        Loop(j, MAX_CHAIN_CIRCLES) {
          Chain_Circle* cc = &gs->chain_circles[j];
          if(j == i) continue;
          if(!cc->is_active) continue;
          if(cc->is_infected) continue;
          
          if(check_circle_vs_circle(c->pos, c->radius*c->infection, cc->pos, cc->radius)) {
            infect_chain_circle(cc);
          }
        }
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
  f32 delta_time = GetFrameTime();
  
  Loop(i, MAX_SCORE_DOTS) {
    Score_Dot* dot = &gs->score_dots[i];
    if(!dot->is_active) continue;
     
    f32 pulse_target_time = 1.0f/SCORE_DOT_PULSE_FREQ;
    dot->pulse_time += delta_time;
    if(dot->pulse_time > pulse_target_time) {
      dot->pulse_time = 0.0f;
      dot->pulse_radius = 0.0f;   
    }
    
    f32 t = dot->pulse_time/pulse_target_time;
    ease_out_quad(&t);
    dot->pulse_radius = lerp_f32(SCORE_DOT_RADIUS, SCORE_DOT_PULSE_TARGET_RADIUS, t);
    
    dot->life_time += delta_time;
    if(dot->life_time > SCORE_DOT_LIFETIME) remove_score_dot(dot);
  }
}


void update_entities(void) {
  Game_State* gs = get_game_state();
  
  // update entities
  Loop(i, gs->entity_count) {
    Entity* entity = &gs->entities[i];
    switch(entity->type) {
      case Entity_Type_Player:            { update_player(entity);            } break;
      case Entity_Type_Laser_Turret:      { update_laser_turret(entity);      } break;
      case Entity_Type_Triple_Gun_Turret: { update_triple_gun_turret(entity); } break;
      case Entity_Type_Goon:              { update_goon(entity);              } break;
      case Entity_Type_Chain_Activator:   { update_chain_activator(entity);   } break;
      case Entity_Type_Infector:          { update_infector(entity);          } break;
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

void pause_audio() {
  Game_State* gs = get_game_state();

  if(gs->song_index != -1) {
    PauseMusicStream(gs->songs[gs->song_index]);
  }  
}

void resume_audio() {
  Game_State* gs = get_game_state();
  if(gs->song_index != -1) {
    ResumeMusicStream(gs->songs[gs->song_index]);
  }
}

void update_audio() {
  Game_State* gs = get_game_state();
  f32 delta_time = GetFrameTime();
  
  if(gs->is_level_music_done) return;
  
  if(gs->song_index == -1) {
    gs->song_index = 0;
    gs->song_timer = timer_start(GetMusicTimeLength(gs->songs[gs->song_index]));
    PlayMusicStream(gs->songs[gs->song_index]);
  }
  
  Music curr = gs->songs[gs->song_index];
  UpdateMusicStream(curr);

  if(timer_step(&gs->song_timer, delta_time)) {
    StopMusicStream(curr);
    if(gs->song_index + 1 < ArrayCount(gs->songs)) {
      gs->song_index += 1;
      curr = gs->songs[gs->song_index];
      gs->song_timer = timer_start(GetMusicTimeLength(curr));
      PlayMusicStream(curr);
    }else {
      gs->is_level_music_done = true;
      StopMusicStream(curr);
    }
  }
}

void update_level() {
  Game_State* gs = get_game_state();
  f32 delta_time = GetFrameTime();
  
  gs->level_time_passed += delta_time;
  f32 level_completion = gs->level_time_passed/gs->level_duration;
  
  Vec2 goon_time_range  = {};
  Vec2 goon_count_range = {};
  
  Vec2 lturret_time_range   = {};
  Vec2 tturret_time_range   = {};
  Vec2 activator_time_range = {};
  Vec2 infector_time_range  = {};
  
  b32 allow_goons = false;
  b32 allow_lturret = false;
  b32 allow_tturret = false;
  b32 allow_activator = false;
  b32 allow_infector = false;

  if(level_completion > 0.95f) {
    return;
  }
  else if(level_completion > 0.75f) {
    allow_goons = true; allow_activator = true; allow_lturret = true; allow_tturret = true;
    allow_infector = true;
    
    goon_time_range  = {5, 7};
    goon_count_range = {2, 4};
    
    lturret_time_range   = {12, 15};
    tturret_time_range   = {12, 15};
    activator_time_range = {12, 15};
    infector_time_range  = {16, 20};
  }
  else if(level_completion > 0.5f) {
    allow_goons = true; allow_activator = true; allow_lturret = true; allow_tturret = true;
    allow_infector = true;
    
    goon_time_range  = {5, 7};
    goon_count_range = {2, 4};
    
    lturret_time_range   = {12, 15};
    tturret_time_range   = {12, 15};
    activator_time_range = {12, 15};
    infector_time_range  = {18, 24};

  }
  else if(level_completion > 0.25f) {
    allow_goons = true; allow_activator = true; allow_lturret = true; allow_tturret = true;
    
    goon_time_range  = {5, 7};
    goon_count_range = {2, 4};
    
    lturret_time_range   = {12, 15};
    tturret_time_range   = {12, 15};
    activator_time_range = {12, 15};  
  }else if(level_completion > 0.15f) {
    allow_goons = true; allow_activator = true; allow_lturret = true; allow_tturret = true;
    
    goon_time_range  = {5, 7};
    goon_count_range = {2, 4};
    
    lturret_time_range   = {12, 15};
    tturret_time_range   = {12, 15};
    activator_time_range = {12, 15};
  }
  else if(level_completion > 0.05f) {
    allow_goons = true;
    allow_activator = true;
    
    goon_time_range  = {3, 6};
    goon_count_range = {2, 4};
    
    activator_time_range = {10, 12};
  }
  else if(gs->level_played_times > 1){
    allow_goons = true;
    allow_activator = true;
    
    goon_time_range  = {3, 6};
    goon_count_range = {2, 4};
    
    activator_time_range = {10, 12};
  }
  
  if(!gs->are_spawn_timers_init) {
    gs->spawn_timer.goon          = timer_start(vec2_lerp_x_to_y(goon_time_range,      random_f32()));
    gs->spawn_timer.laser_turret  = timer_start(vec2_lerp_x_to_y(lturret_time_range,   random_f32()));
    gs->spawn_timer.triple_turret = timer_start(vec2_lerp_x_to_y(tturret_time_range,   random_f32()));
    gs->spawn_timer.activator     = timer_start(vec2_lerp_x_to_y(activator_time_range, random_f32()));
    gs->spawn_timer.infector      = timer_start(vec2_lerp_x_to_y(infector_time_range, random_f32()));

    gs->are_spawn_timers_init     = true;
  }

  b32 should_spawn_goons     = false;
  b32 should_spawn_lturret   = false;
  b32 should_spawn_tturret   = false;
  b32 should_spawn_activator = false;
  b32 should_spawn_infector  = false;
  
  if(timer_step(&gs->spawn_timer.goon, delta_time)) {
    should_spawn_goons = allow_goons;
    gs->spawn_timer.goon = timer_start(vec2_lerp_x_to_y(goon_time_range, random_f32()));
  }
  if(timer_step(&gs->spawn_timer.laser_turret, delta_time)) {
    should_spawn_lturret = allow_lturret;
    gs->spawn_timer.laser_turret = timer_start(vec2_lerp_x_to_y(lturret_time_range, random_f32()));
  }
  if(timer_step(&gs->spawn_timer.triple_turret, delta_time)) {
    should_spawn_tturret = allow_tturret;
    gs->spawn_timer.triple_turret = timer_start(vec2_lerp_x_to_y(tturret_time_range, random_f32()));
  }
  if(timer_step(&gs->spawn_timer.activator, delta_time)) {
    should_spawn_activator = allow_activator;
    gs->spawn_timer.activator = timer_start(vec2_lerp_x_to_y(activator_time_range, random_f32()));
  }
  if(timer_step(&gs->spawn_timer.infector, delta_time)) {
    should_spawn_infector = allow_infector;
    gs->spawn_timer.infector = timer_start(vec2_lerp_x_to_y(infector_time_range, random_f32()));
  }
    
  if(should_spawn_goons) {
    // Basic goon formations
    char* column = {
      "*"
      "*"
      "#"
      "*"
      "*"
    };
    
    char* row = {
      "**#**"
    };
    
    s32 min = (s32)goon_count_range.min;
    s32 max = (s32)goon_count_range.max;
    s32 count = random_range(min, max);
    Loop(i, count) {
      if(random_chance(4)) {
        spawn_goon_formation(row, 5, 1);
      }else {
        spawn_goon_formation(column, 1, 5);
      }
    }    
    
    // Heavy goon formations
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
    
    if(random_chance(2)) {
      char* big_guys[] = {tank, ufo, thruster};
      char* big_guy = big_guys[random_range(0, 3)];
      
      spawn_goon_formation(big_guy, 5, 5);
    }
  }
  
  if(should_spawn_lturret)   new_entity(Entity_Type_Laser_Turret);
  if(should_spawn_tturret)   new_entity(Entity_Type_Triple_Gun_Turret);
  if(should_spawn_activator) new_entity(Entity_Type_Chain_Activator);
  if(should_spawn_infector)  new_entity(Entity_Type_Infector);
}

char* chain_activator_line0 = "Shoot only once to activate";
char* chain_activator_line1 = "Shoot multiple times";
char* chain_activator_line2 = "Shoot to cause chain a reaction";

void set_level_to_initial_state() {
  Game_State* gs = get_game_state();
  
  Loop(i, gs->entity_count) gs->entities[i].base.is_active = false;
  gs->entity_count = 0;
  
  Loop(i, MAX_PROJECTILES)   gs->projectiles[i].is_active   = false;
  Loop(i, MAX_CHAIN_CIRCLES) gs->chain_circles[i].is_active = false;
  Loop(i, MAX_EXPLOSIONS)    gs->explosions[i].is_active    = false;
  Loop(i, MAX_SCORE_DOTS)    gs->score_dots[i].is_active    = false;
  
  f32 level_silence_time = 15.0f;
  gs->level_duration = GetMusicTimeLength(gs->songs[0]) + GetMusicTimeLength(gs->songs[1]) + level_silence_time;
  gs->level_time_passed = 0.0f;

  gs->song_index = -1;
  gs->is_level_music_done = false;
  
  StopMusicStream(gs->songs[0]);
  StopMusicStream(gs->songs[1]);

  gs->score = 0;
  gs->got_high_score = false;
  
  gs->are_spawn_timers_init = false;  
  
  Player* player = (Player*)new_entity(Entity_Type_Player);
  player->pos = get_screen_center();
  player->radius = PLAYER_RADIUS;
  player->shoot_cooldown_timer = timer_start(PLAYER_SHOOT_COOLDOWN);
  entity_set_hit_points(player, PLAYER_HIT_POINTS);
  
  if(gs->level_played_times == 0) {
    gs->show_game_controls_timer = timer_start(5.0f);
    
    // Shoot once
    {
      Chain_Activator* activator = (Chain_Activator*)new_entity(Entity_Type_Chain_Activator);
      activator->for_tutorial_purposes = true;
      activator->text_line = chain_activator_line0;
      activator->pos = {WINDOW_WIDTH/2, -CHAIN_ACTIVATOR_START_RADIUS*5};
      activator->dir = {0, 1};
    };
    
    {
      Chain_Activator* activator = (Chain_Activator*)new_entity(Entity_Type_Chain_Activator);
      activator->for_tutorial_purposes = true;
      activator->text_line = chain_activator_line0;
      activator->pos = {-CHAIN_ACTIVATOR_START_RADIUS*12, WINDOW_HEIGHT/2};
      activator->dir = {1, 0};
    };
    
    // Shoot multiple times
    {
      Chain_Activator* activator = (Chain_Activator*)new_entity(Entity_Type_Chain_Activator);
      activator->for_tutorial_purposes = true;
      activator->text_line = chain_activator_line1;
      activator->pos = {WINDOW_WIDTH + CHAIN_ACTIVATOR_START_RADIUS*22, WINDOW_HEIGHT/2};
      activator->dir = {-1, 0};
    };
    
    {
      Chain_Activator* activator = (Chain_Activator*)new_entity(Entity_Type_Chain_Activator);
      activator->for_tutorial_purposes = true;
      activator->text_line = chain_activator_line1;
      activator->pos = {WINDOW_WIDTH/2, WINDOW_HEIGHT + CHAIN_ACTIVATOR_START_RADIUS*22};
      activator->dir = {0, -1};
    };
    
    // Shoot to cause a chain reaction
    {
      Chain_Activator* activator = (Chain_Activator*)new_entity(Entity_Type_Chain_Activator);
      activator->for_tutorial_purposes = true;
      activator->text_line = chain_activator_line2;
      activator->pos = {WINDOW_WIDTH/2, -CHAIN_ACTIVATOR_START_RADIUS*35};
      activator->dir = {0, 1};
    };
    
    {
      Chain_Activator* activator = (Chain_Activator*)new_entity(Entity_Type_Chain_Activator);
      activator->for_tutorial_purposes = true;
      activator->text_line = chain_activator_line2;
      activator->pos = {-CHAIN_ACTIVATOR_START_RADIUS*40, WINDOW_HEIGHT/2};
      activator->dir = {1, 0};
    };
    
    {
      Chain_Activator* activator = (Chain_Activator*)new_entity(Entity_Type_Chain_Activator);
      activator->for_tutorial_purposes = true;
      activator->text_line = chain_activator_line2;
      activator->pos = {WINDOW_WIDTH + CHAIN_ACTIVATOR_START_RADIUS*40, WINDOW_HEIGHT/2};
      activator->dir = {-1, 0};
    };
    
    {
      Chain_Activator* activator = (Chain_Activator*)new_entity(Entity_Type_Chain_Activator);
      activator->for_tutorial_purposes = true;
      activator->text_line = chain_activator_line2;
      activator->pos = {WINDOW_WIDTH/2, WINDOW_HEIGHT + CHAIN_ACTIVATOR_START_RADIUS*35};
      activator->dir = {0, -1};
    };
  }
  
  gs->level_played_times += 1;
}

void update_game(void) {
  Game_State* gs = get_game_state();
  f32 delta_time = GetFrameTime();
    
  f64 start_time = GetTime();
  
  update_explosion_polygon();
  
  update_entities();  
  actually_remove_entities();
  
  update_score_dots();
  update_particles();
  update_projectiles();
  update_explosions();
  update_chain_circles();
  
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
  //gs->show_debug_info = false;
  if(!gs->show_debug_info) return;
  
  Vec2 pos = {10, 10};
  f32 font_size = 24;
  
  draw_text(gs->small_font, "Debug Info:", pos, WHITE_VEC4);
  pos.y += font_size;
  
  char* score_text = (char*)TextFormat("entity_count: %d\n", gs->entity_count);
  draw_text(gs->small_font, score_text, pos, WHITE_VEC4);
  pos.y += font_size;
  
  score_text = (char*)TextFormat("projectile_count: %d\n", gs->active_projectile_count);
  draw_text(gs->small_font, score_text, pos, WHITE_VEC4);
  pos.y += font_size;
  
  
  score_text = (char*)TextFormat("chain_circle_count: %d\n", gs->active_chain_circle_count);
  draw_text(gs->small_font, score_text, pos, WHITE_VEC4);
  pos.y += font_size;
  
  
  score_text = (char*)TextFormat("score_dot_count: %d\n", gs->active_score_dot_count);
  draw_text(gs->small_font, score_text, pos, WHITE_VEC4);
  pos.y += font_size;
  
  
  score_text = (char*)TextFormat("explosion_count: %d\n", gs->active_explosion_count);
  draw_text(gs->small_font, score_text, pos, WHITE_VEC4);
  pos.y += font_size;
  
  
  f64 update_ms  = gs->update_time*1000.0f;
  s32 update_fps = (s32)(1.0f/gs->update_time);
  score_text = (char*)TextFormat("update_ms:  %.4f[%d fps]\n", update_ms, update_fps);
  draw_text(gs->small_font, score_text, pos, WHITE_VEC4);
  pos.y += font_size;
  
  
  f64 draw_ms  = gs->draw_time*1000.0f;
  s32 draw_fps = (s32)(1.0f/gs->draw_time);
  score_text = (char*)TextFormat("draw_ms:      %.4f[%d fps]\n", draw_ms, draw_fps);
  draw_text(gs->small_font, score_text, pos, WHITE_VEC4);
  pos.y += font_size;
  
  
  f64 frame_ms  = GetFrameTime()*1000.0f;
  s32 frame_fps = (s32)(1.0f/GetFrameTime() + 0.5f);
  score_text = (char*)TextFormat("frame_ms:   %.4f[%d fps]\n", frame_ms, frame_fps);
  draw_text(gs->small_font, score_text, pos, WHITE_VEC4);
  pos.y += font_size;
}

void draw_health_bar(Entity* the_entity) {
  Entity_Base* entity = (Entity_Base*)the_entity;
  
  Vec2 top_left = entity->pos - vec2(1,1)*entity->radius;
  
  f32 hp_bar_h   = 10;
  f32 hp_bar_pad = 4;
  
  f32 life = (f32)entity->hit_points/(f32)entity->initial_hit_points;
  f32 hp_bar_w = entity->radius*2.0f*life;
  
  Vec2 hp_bar_dim = vec2(hp_bar_w, hp_bar_h);
  Vec2 hp_bar_pos = top_left - vec2(0, hp_bar_h + hp_bar_pad);
  
  draw_quad(hp_bar_pos, hp_bar_dim, RED_VEC4);
  draw_quad_outline(hp_bar_pos, hp_bar_dim, 2.0f, BLACK_VEC4);
}


void draw_laser_turret(Entity* entity) {
  Laser_Turret* turret = (Laser_Turret*)entity;
  Vec2 dim = vec2(1, 1)*turret->radius*2;
  
  // Laser
  if(turret->state == Entity_State_Telegraphing) {  
    Vec2 v = vec2(turret->shoot_angle);
    draw_line(turret->pos, turret->pos + v*2000.0f, 2.0f, {1,1,1,0.5f});
  }
  
  // Gun  
  f32 radius_t = turret->radius/LASER_TURRET_RADIUS;
  Vec2 gun_dim = {LASER_TURRET_GUN_HEIGHT, LASER_TURRET_GUN_WIDTH};
  gun_dim *= radius_t;
  
  f32 offset_to_gun = turret->radius + gun_dim.width/2;
  Vec2 gun_pos = turret->pos + vec2(turret->rotation)*offset_to_gun;
  draw_quad(gun_pos - gun_dim*0.5f, gun_dim, turret->rotation, BLACK_VEC4);

  // Turret
  draw_quad(turret->pos - dim*0.5f, dim, turret->rotation, BLACK_VEC4);
  f32 f = 0.85f;
  draw_quad(turret->pos - dim*0.5f*f, dim*f, turret->rotation, turret->color);
  
  b32 show_health = timer_is_active(turret->health_bar_display_timer);
  if(show_health) draw_health_bar((Entity*)turret);
}

void draw_triple_gun_turret(Entity* entity) {
  Game_State* gs = get_game_state();
  
  Triple_Gun_Turret* turret = (Triple_Gun_Turret*)entity;
  Vec2 dim = vec2(1, 1)*turret->radius*2;
  
  f32 radius_t = turret->radius/TRIPLE_GUN_TURRET_RADIUS;
  Vec2 gun_dim = {TRIPLE_GUN_TURRET_GUN_HEIGHT, TRIPLE_GUN_TURRET_GUN_WIDTH};
  gun_dim *= radius_t;
  
  f32 angle_step = TRIPLE_GUN_TURRET_GUN_ANGLE_STEP;
  f32 angle = -angle_step;
  Loop(i, 3) {
    Vec2 dir = vec2(turret->rotation + angle);
    Vec2 pos = turret->pos + dir*(turret->radius + gun_dim.height/2.5f);
    draw_quad(pos - gun_dim*0.5f, gun_dim, turret->rotation + angle, BLACK_VEC4);
    angle += angle_step;
  }
  draw_quad(gs->chain_activator_texture, turret->pos - dim*0.5f, dim, turret->rotation + Pi32/2, turret->color);
  
  b32 show_health = timer_is_active(turret->health_bar_display_timer);
  if(show_health) draw_health_bar((Entity*)turret);
}


void draw_butterfly(Vec2 pos, f32 scale, f32 rot, f32 y_offset, Vec4 color) {
  Game_State* gs = get_game_state();
  
  Vec2* top_wing = gs->butterfly_top_wing;
  Vec2* bottom_wing = gs->butterfly_bottom_wing;

  Vec2 offset = {0, y_offset};
  Vec2 top_offsets[5]    = {{0, 0}, offset, offset, offset, {0,0}};
  Vec2 bottom_offsets[5] = {{0, 0}, {0,0}, offset, offset, offset};
  
  for(s32 i = 1; i < 5 - 1; i += 1) {

    Vec2 a = top_wing[0] + top_offsets[0];
    Vec2 b = top_wing[i] + top_offsets[i];
    Vec2 c = top_wing[i+1] + top_offsets[i+1];
    
    Vec2 v1 = pos + vec2_rotate(a, rot)*scale;
    Vec2 v2 = pos + vec2_rotate(b, rot)*scale;
    Vec2 v3 = pos + vec2_rotate(c, rot)*scale;

    draw_triangle(v3, v2, v1, vec4_fade_alpha(color, 0.5f));     
    draw_triangle_outline(v3, v2, v1, color);

    // flipped on x
    v1 = pos + vec2_rotate({-a.x, a.y}, rot)*scale;
    v2 = pos + vec2_rotate({-b.x, b.y}, rot)*scale;
    v3 = pos + vec2_rotate({-c.x, c.y}, rot)*scale;
    draw_triangle(v1, v2, v3, vec4_fade_alpha(color, 0.5f));     
    draw_triangle_outline(v3, v2, v1, color);
  }
  
  
  for(s32 i = 1; i < 5 - 1; i += 1) {
    Vec2 a = bottom_wing[0] + bottom_offsets[0];
    Vec2 b = bottom_wing[i] + bottom_offsets[i];
    Vec2 c = bottom_wing[i+1] + bottom_offsets[i+1];
    
    Vec2 v1 = pos + vec2_rotate(a, rot)*scale;
    Vec2 v2 = pos + vec2_rotate(b, rot)*scale;
    Vec2 v3 = pos + vec2_rotate(c, rot)*scale;

    draw_triangle(v3, v2, v1, vec4_fade_alpha(color, 0.5f));     
    draw_triangle_outline(v3, v2, v1, color);

    // flipped on x
    v1 = pos + vec2_rotate({-a.x, a.y}, rot)*scale;
    v2 = pos + vec2_rotate({-b.x, b.y}, rot)*scale;
    v3 = pos + vec2_rotate({-c.x, c.y}, rot)*scale;
    draw_triangle(v1, v2, v3, vec4_fade_alpha(color, 0.5f));     
    draw_triangle_outline(v3, v2, v1, color);
  }
}

void draw_infector_shape(Vec2 pos, f32 radius, Vec4 color = RED_VEC4, Vec4 outline_color = BLACK_VEC4) {
  Vec2 dim = vec2(2,2)*radius;
  Vec2 outline_dim = dim + vec2(4,4);
  
  f32 angle_step = (2*Pi32)/3;
  f32 angle = 0.0f;
  Loop(i, 3) {
    draw_quad(pos - outline_dim*0.5f, outline_dim, angle, outline_color);
    angle += angle_step;
  }
  
  angle = 0.0f;
  Loop(i, 3) {
    draw_quad(pos - dim*0.5f, dim, angle, color);
    angle += angle_step;
  }
}

void draw_infector(Entity* entity) {
  Infector* infector = (Infector*)entity;
  f32 scale = infector->radius + infector->wobble*8.0f;
  draw_infector_shape(infector->pos, scale);
  draw_infector_shape(infector->pos, scale*0.65f, vec4(0xffff8519));
  
  b32 show_health = timer_is_active(infector->health_bar_display_timer);
  if(show_health) draw_health_bar((Entity*)infector);
}

void draw_player(Entity* entity) {
  Player* player = (Player*)entity;
  
  if(player->hit_points <= 0) return;
  
  Vec2 pos = player->pos;
  Vec2 dim = vec2(2,2)*player->radius; 
  
  f32 shoot_indicator_scale = player->shoot_indicator*10.0f;
  
  f32 wobble_scale = player->wobble*player->wobble_scale;
  f32 scale = 5.0f*player->radius + wobble_scale + shoot_indicator_scale;
  
  Vec4 color = vec4_lerp(WHITE_VEC4, RED_VEC4, player->wobble);
  draw_butterfly(pos, scale, player->turn_angle, player->flap, color);
}

void draw_entities(void) {
  Game_State* gs = get_game_state();

  Loop(i, gs->entity_count) {
    Entity* the_entity = &gs->entities[i];
    Entity_Base* entity = (Entity_Base*)the_entity;
  
    switch(entity->type) {
      case Entity_Type_Player: {
        draw_player(the_entity);
      }break;
      case Entity_Type_Goon: {
        Vec2 dim = vec2(1, 1)*entity->radius*2;
        f32 thickness = 3.0f;
        
        f32 scale = 1.0f - thickness/entity->radius;
        draw_quad(entity->pos - dim*0.5f, dim, entity->rotation, GOON_OUTLINE_COLOR);        
        draw_quad(entity->pos - dim*0.5f*scale, dim*scale, entity->rotation, entity->color);
        
        b32 show_health = timer_is_active(entity->health_bar_display_timer);
        if(show_health) draw_health_bar(the_entity);
      }break;
      case Entity_Type_Laser_Turret:      { draw_laser_turret(the_entity);      }break;
      case Entity_Type_Triple_Gun_Turret: { draw_triple_gun_turret(the_entity); }break;
      case Entity_Type_Infector:          { draw_infector(the_entity);          }break;
      case Entity_Type_Chain_Activator: {
        Chain_Activator* activator = (Chain_Activator*)entity;
        
        Vec2 dim = vec2(2, 2)*activator->radius;
        Vec2 pos = activator->pos - dim*0.5f;
        draw_quad(gs->chain_activator_texture, pos, dim, activator->rotation, activator->color);
        
        Vec2 orbital_dim = vec2(2,2)*activator->orbital_radius;
        s32 orbital_count = ArrayCount(activator->orbitals);
        f32 angle = activator->orbital_global_rotation;
        f32 angle_step = (2.0f*Pi32)/(f32)orbital_count;
        Loop(i, orbital_count) {
          if(!activator->orbitals[i].active) continue;
          Vec2 local_pos  = vec2(angle);
          Vec2 global_pos = activator->pos + local_pos*(activator->radius + activator->orbital_radius);
          f32 rot = activator->orbitals[i].rotation;
          
          draw_quad(gs->chain_activator_texture, global_pos - orbital_dim*0.5f, orbital_dim, rot, activator->color);
          angle += angle_step;
        }        
        
        if(activator->for_tutorial_purposes) {
          char* text = activator->text_line;
          Vector2 tdim = MeasureTextEx(gs->small_font, text, gs->small_font.baseSize, 0);
          Vec2 tpos = {activator->pos.x - tdim.x/2, pos.y - dim.height/2 - gs->small_font.baseSize};
          draw_text(gs->small_font, text, tpos, WHITE_VEC4);
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
    
    switch(p->from_type) {
      case Entity_Type_Player: {
        draw_quad(p->pos - dim*0.5f, dim, p->rotation, p->color);
      } break;
      case Entity_Type_Triple_Gun_Turret: {
        draw_quad(p->pos - dim*0.5f, dim, p->rotation, p->color);
      }break;
      case Entity_Type_Infector: {
        draw_infector_shape(p->pos, p->radius);
        draw_quad(p->pos - dim*0.5f, dim, p->rotation, p->color);
      }break;
      case Entity_Type_Laser_Turret: {
        Vec4 color = {1,1,1,1};
        
        if(p->life_timer.passed_time > LASER_TURRET_PROJECTILE_FADE_AFTER) {
          f32 fade_time = LASER_TURRET_PROJECTILE_LIFETIME - LASER_TURRET_PROJECTILE_FADE_AFTER;
          
          f32 t = 1.0f - (p->life_timer.passed_time - LASER_TURRET_PROJECTILE_FADE_AFTER)/fade_time;
          ease_out_quad(&t);
        
          dim.height *= t;
          if(dim.height < 20) dim.height = 20;         
          color.a = t;   
        }
        
        draw_quad(gs->laser_bullet_texture, p->pos - dim*0.5f, dim, p->rotation, color);
      }break;
    }
  }
}


void draw_chain_circles(void) {
  Game_State* gs = get_game_state();

  Loop(i, MAX_CHAIN_CIRCLES){
    Chain_Circle* c = &gs->chain_circles[i];
    if(!c->is_active) continue;
    
    Vec2 dim = vec2(1,1)*2*c->radius;
    Vec2 pos = c->pos - dim*0.5f;
  
    Vec4 color = WHITE_VEC4;
    if(c->life_prolong_time > 0.0f) color = YELLOW_VEC4;
    draw_quad(gs->chain_circle_texture, pos, dim, color);
  
    f32 t = Max(c->life_time,0.0f)/MAX_CHAIN_CIRCLE_LIFE_TIME;
  
    t = -t*t + 2*t; // Ease out quadratic
  
    Vec2 indicator_dim = dim*t*0.7f;
    Vec2 indicator_pos = c->pos - indicator_dim*0.5f;
    draw_quad(gs->chain_circle_texture, indicator_pos, indicator_dim, vec4_fade_alpha(color, 0.25f));
    
    if(c->is_infected) {
      Vec2 infection_dim = vec2(2,2)*c->radius*c->infection;
      Vec2 infection_pos = c->pos - infection_dim*0.5f;
      draw_quad(gs->chain_circle_texture, infection_pos, infection_dim, vec4_fade_alpha(RED_VEC4, 0.85f));
    }    
  }
}

void draw_score_dots(void) {
  Game_State* gs = get_game_state();
  Vec2 dot_dim = vec2(2,2)*SCORE_DOT_RADIUS;
  
  f32 outline_thickness = 2.0f;
  
  Loop(i, MAX_SCORE_DOTS) {
    Score_Dot* dot = &gs->score_dots[i];
    if(!dot->is_active) continue;
  
    
    Vec4 outter_color = WHITE_VEC4;
    
    f32 inner_alpha = 0.5f;
    Vec4 inner_color = vec4_fade_alpha(outter_color, inner_alpha);  
        
    Vec2 center_pos = dot->pos - dot_dim*0.5f;
    
    
    if(dot->is_special) {
      f32 cos_freq = 1.0f/(2*Pi32);
      f32 cos_offset = Pi32;
      
      f32 x = GetTime();
      f32 freq = SCORE_DOT_BLINK_FREQ;
      f32 t = (cosf(x*freq - cos_offset) + 1)/2.0f;
      inner_color.a = lerp_f32(inner_alpha, 1.0f, t);
    }
    
    
    if(dot->life_time > SCORE_DOT_FADE_AFTER) {
      f32 fade_time = (SCORE_DOT_LIFETIME - SCORE_DOT_FADE_AFTER);
      f32 fade = 1.0f - (dot->life_time - SCORE_DOT_FADE_AFTER)/fade_time;
      
      ease_out_quad(&fade);     
      inner_color.a  *= fade;
      outter_color.a *= fade;
    }
    
    draw_quad(center_pos, dot_dim, inner_color);
    draw_quad_outline(center_pos, dot_dim, outline_thickness, outter_color);
    
    if(dot->is_special) {
      Vec2 pulse_dim = vec2(2,2)*dot->pulse_radius;
      draw_quad_outline(dot->pos - pulse_dim*0.5f, pulse_dim, 2.0f, inner_color);
    }
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

void draw_level_completion_bar(void) {
  Game_State* gs = get_game_state();
  
  f32 bar_width  = WINDOW_WIDTH*0.9f;
  f32 bar_height = 10.0f;
  f32 bottom_padding = 10.0f;
    
  Vec2 bar_dim = {bar_width, bar_height};
  Vec2 bar_pos = {WINDOW_WIDTH/2 - bar_width/2, WINDOW_HEIGHT - bar_height - bottom_padding};
  
  f32 cursor_width  = bar_height;
  f32 cursor_height = bar_height + 5;
  
  Vec2 cursor_dim = {cursor_width, cursor_height};  
  
  f32 t = gs->level_time_passed/gs->level_duration;
  f32 cursor_x = lerp_f32(bar_pos.x, bar_pos.x + bar_dim.width - cursor_dim.width, t);
  f32 cursor_y = bar_pos.y + bar_height/2 - cursor_height/2;
  Vec2 cursor_pos = {cursor_x, cursor_y};
  
  f32 thickness = 2.0f;
  
  draw_quad_outline(bar_pos,    bar_dim,    thickness, {1,1,1,0.5f});
  draw_quad(cursor_pos, cursor_dim, {1,1,1,0.75f});
}

void draw_score_and_life(void) {
  Game_State* gs = get_game_state();
  Player* player = get_player();
  
  char* score_text = (char*)TextFormat("%d\n", gs->score);
  Vector2 dim = MeasureTextEx(gs->big_font, score_text, 48, 0);
  draw_text(gs->medium_font, score_text, {WINDOW_WIDTH/2 - dim.x/2, 5}, {1,1,1,0.75f});
  
  char* life_text = (char*)TextFormat("Life: %d", player->hit_points);
  draw_text(gs->small_font, life_text, {10, 5 + 48/2 - 24/2}, {1,1,1,0.75f});
}

void draw_game(void) {
  Game_State* gs = get_game_state();
  
  ClearBackground(rl_color(0.2f, 0.2f, 0.35f, 1.0f));
  
  f64 start_time = GetTime();
  
  draw_entities();

  draw_score_dots();
  draw_particles();
  draw_projectiles();
  
  draw_chain_circles();
  draw_explosions();
  draw_level_completion_bar();
  draw_score_and_life();
  
  f64 end_time = GetTime();
  gs->draw_time = end_time - start_time;
  draw_debug_info();
}

s32 get_option_navigation_direction() {
  s32 r = 0;
  
  if(IsKeyPressed(KEY_W)) r -= 1;
  if(IsKeyPressed(KEY_S)) r += 1;
  
  if(IsKeyPressed(KEY_UP))   r -= 1;
  if(IsKeyPressed(KEY_DOWN)) r += 1;
  
  if(IsGamepadAvailable(0)) {
    if(IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP))   r -= 1;
    if(IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) r += 1;
  }
  
  r = Sign(r);
  return r;
}

b32 check_confirmation_press() {
  if(IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) return true;
  if(IsGamepadAvailable(0)) {
    if(IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) return true;
  }
  return false;
}

b32 check_escape_press() {
  b32 r = (IsKeyPressed(KEY_ESCAPE) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT));
  return r;
}

void change_game_screen(Game_Screen screen) {
  Game_State* gs = get_game_state();
  gs->game_screen = screen;
  gs->has_entered_game_screen = false;
  gs->option_index = 0;  
}

s32 get_vertical_navigation_dir() {
  s32 r = 0;
  
  if(IsKeyPressed(KEY_W)) r -= 1;
  if(IsKeyPressed(KEY_S)) r += 1;
  
  if(IsKeyPressed(KEY_UP))   r -= 1;
  if(IsKeyPressed(KEY_DOWN)) r += 1;
  
  if(IsGamepadAvailable(0)) {
    if(IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP))   r -= 1;
    if(IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) r += 1;
  }
  
  r = Sign(r);
  return r;
}

s32 get_horizontal_navigation_dir() {
  s32 r = 0;
  
  if(IsKeyPressed(KEY_A)) r -= 1;
  if(IsKeyPressed(KEY_D)) r += 1;
  
  if(IsKeyPressed(KEY_LEFT))   r -= 1;
  if(IsKeyPressed(KEY_RIGHT)) r += 1;
  
  if(IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT))   r -= 1;
  if(IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) r += 1;
  
  r = Sign(r);
  return r;
}


void do_game_screen(void) {
  Game_State* gs = get_game_state();
  Player* player = get_player();

  b32 is_level_finished = gs->level_time_passed > gs->level_duration;
  
  if(player->hit_points <= 0)  {
    change_game_screen(Game_Screen_Death);   
  }
  else if(check_escape_press()) {
    change_game_screen(Game_Screen_Paused);
  }
  else if(is_level_finished)  {
    change_game_screen(Game_Screen_Win);
  }
    
  update_audio();
  update_level();
  update_game();

  timer_step(&gs->show_game_controls_timer, GetFrameTime());
  b32 show_game_controls = timer_is_active(gs->show_game_controls_timer);
    
  BeginDrawing();
  draw_game();
  
  if(show_game_controls) {
    f32 base_y = 300.0f;
    f32 alpha = 1.0f - timer_procent(gs->show_game_controls_timer);
    ease_out_quad(&alpha);
    
    f32 right_x = WINDOW_WIDTH/4;
    char* right_lines[] = {
      "Movement:",
      " ",
      "WASD",
      "D-pad",
      "Right stick",
      "\0",
    };
    
    Font font = gs->small_font;
    
    s32 i = 0;
    f32 y = base_y;
    while(right_lines[i][0] != '\0') {
      Vector2 dim = MeasureTextEx(font, right_lines[i], font.baseSize, 0);
      Vec2 pos = vec2(right_x - dim.x/2, y);
      draw_text(font, right_lines[i], pos, vec4_fade_alpha(WHITE_VEC4, alpha));
      y += font.baseSize;
      i += 1;
    }
    
    f32 left_x = WINDOW_WIDTH/2 + WINDOW_WIDTH/4;
    char* left_lines[] = {
      "Shooting:",
      " ",
      "Arrows",
      "Buttons",
      "Left Stick",
      "\0"
    };
    
    i = 0;
    y = base_y;
    while(left_lines[i][0] != '\0') {
      Vector2 dim = MeasureTextEx(font, left_lines[i], font.baseSize, 0);
      Vec2 pos = vec2(left_x - dim.x/2, y);
      draw_text(font, left_lines[i], pos, vec4_fade_alpha(WHITE_VEC4, alpha));
      y += font.baseSize;
      i += 1;
    }
    
  }
  
  EndDrawing();
}

void do_menu_screen(void) {
  Game_State* gs = get_game_state();
  
  char* options[3] = { "Start","Credits","Volume"};  
  gs->option_index = Wrap(gs->option_index + get_vertical_navigation_dir(), 0, ArrayCount(options) - 1);
  char* option = options[gs->option_index];

  if(check_confirmation_press()) {
    if(cstr_equal(option, "Start")) {
      set_level_to_initial_state();
      change_game_screen(Game_Screen_Game);      
    }
    else if(cstr_equal(option, "Credits")) {
      change_game_screen(Game_Screen_Credits);
    }
  }
  
  if(cstr_equal(option, "Volume")) {
    gs->master_volume = Clamp(gs->master_volume + get_horizontal_navigation_dir(), 0, MAX_MASTER_VOLUME);
    f32 volume = (f32)gs->master_volume/(f32)MAX_MASTER_VOLUME;
    SetMasterVolume(volume);
  }
  
  BeginDrawing();
  
  draw_quad({0, 0}, {WINDOW_WIDTH, WINDOW_HEIGHT}, {0.1f, 0.1f, 0.5f, 1.0f});
  draw_text_centered(gs->big_font, "Butterfly", 100, WHITE_VEC4);
  
  f32 y = 500;
  f32 y_pad = 10.0f;
  Loop(i, ArrayCount(options)) {
    b32 is_selected = (i == gs->option_index);
    
    Vec4 color = WHITE_VEC4;
    if(is_selected) {
      f32 x = GetTime()*12.0f;
      f32 t = (cosf(x) + 1)/2.0f;
      color = vec4_lerp(WHITE_VEC4, YELLOW_VEC4, t);
    }
    
    b32 is_volume = cstr_equal(options[i], "Volume");
    if(is_volume) {
      Vec2 volume_box = {100, (f32)gs->medium_font.baseSize/2};
      f32 padding = 10.0f;
      
      Vector2 text_dim = MeasureTextEx(gs->medium_font, options[i], gs->medium_font.baseSize, 0);
      f32 width = text_dim.x + volume_box.width + padding;
      
      Vec2 text_pos = {WINDOW_WIDTH/2 - width/2, y};
      draw_text(gs->medium_font, options[i], text_pos, color);
      
      f32 yy = (f32)gs->medium_font.baseSize/2 - volume_box.height/2;
      Vec2 volume_box_pos = {WINDOW_WIDTH/2 - width/2 + text_dim.x + padding, y + yy};
      f32 volume = (f32)gs->master_volume/(f32)MAX_MASTER_VOLUME;
      Vec2 dim = {volume_box.width*volume, volume_box.height};
      draw_quad(volume_box_pos, dim, {1,1,1,0.5});
      draw_quad_outline(volume_box_pos, volume_box, 2.0f, WHITE_VEC4);
    }else {
      draw_text_centered(gs->medium_font, options[i], y, color);
    }
    
    y += gs->medium_font.baseSize + y_pad;
  }
  
  
  char hs_text[255];
  sprintf(hs_text, "High Score: %dx%d", gs->high_score.score, gs->high_score.lives);
  
  f32 hs_pad = 10.0f;
  Vec2 hs_pos = {hs_pad, WINDOW_HEIGHT - gs->small_font.baseSize - hs_pad};
  draw_text(gs->small_font, hs_text, hs_pos, WHITE_VEC4);
  
  f32 flap = cosf(GetTime()*8.0f)*0.065f;
  draw_butterfly(get_screen_center(), 200.0f, 0.0f, flap, WHITE_VEC4);
  EndDrawing();
}

void do_credits_screen(void) {
  Game_State* gs = get_game_state();
  
  if(check_confirmation_press()) {
    change_game_screen(Game_Screen_Menu);
  }
  
  char* lines[] = {
    "Programming, Design, Visuals:",
    "vertex88",
    " ",
    " ",
    "Music:",
    "tebruno99",
    "https://opengameart.org/content/the-rush CC0",
    " ",
    "Snabisch",
    "https://opengameart.org/content/the-treasure-nes-version CC-BY 3.0",
    " ",
    " ",
    "Made with Raylib",
    "\0",
  };
  
  BeginDrawing();
  draw_quad({0, 0}, {WINDOW_WIDTH, WINDOW_HEIGHT}, {0.1f, 0.1f, 0.5f, 1.0f});

  f32 y = 150.0f;
  f32 step = gs->small_font.baseSize;
  
  s32 i = 0;
  while(lines[i][0] != '\0'){
    draw_text_centered(gs->small_font, lines[i], y, WHITE_VEC4);
    y += step;
    i += 1;
  }
  
  f32 x = GetTime()*12.0f;
  f32 t = (cosf(x) + 1)/2.0f;
  Vec4 color = vec4_lerp(WHITE_VEC4, YELLOW_VEC4, t);
  draw_text_centered(gs->medium_font, "Back", WINDOW_HEIGHT - 100.0f, color);
  EndDrawing();
}

void do_pause_screen(void) {
  Game_State* gs = get_game_state();
  
  if(check_escape_press()) change_game_screen(Game_Screen_Game);

  char* options[3] = { "Restart","Menu","Volume"};  
  gs->option_index = Wrap(gs->option_index + get_vertical_navigation_dir(), 0, ArrayCount(options) - 1);
  char* option = options[gs->option_index];

  if(check_confirmation_press()) {
    if(cstr_equal(option, "Restart")) {
      set_level_to_initial_state();
      change_game_screen(Game_Screen_Game);      
    }
    else if(cstr_equal(option, "Menu")) {
      change_game_screen(Game_Screen_Menu);
    }
  }
  
  if(cstr_equal(option, "Volume")) {
    gs->master_volume = Clamp(gs->master_volume + get_horizontal_navigation_dir(), 0, MAX_MASTER_VOLUME);
    f32 volume = (f32)gs->master_volume/(f32)MAX_MASTER_VOLUME;
    SetMasterVolume(volume);
  }

  BeginDrawing();
  draw_game();
  draw_quad({0, 0}, {WINDOW_WIDTH, WINDOW_HEIGHT}, {0.1f, 0.1f, 0.5f, 0.5f});
  draw_text_centered(gs->big_font, "Paused", 200, WHITE_VEC4);
  
  f32 y = 300;
  f32 y_pad = 10.0f;
  Loop(i, ArrayCount(options)) {
    b32 is_selected = (i == gs->option_index);
    
    Vec4 color = WHITE_VEC4;
    if(is_selected) {
      f32 x = GetTime()*12.0f;
      f32 t = (cosf(x) + 1)/2.0f;
      color = vec4_lerp(WHITE_VEC4, YELLOW_VEC4, t);
    }
    
    b32 is_volume = cstr_equal(options[i], "Volume");
    if(is_volume) {
      Vec2 volume_box = {100, (f32)gs->medium_font.baseSize/2};
      f32 padding = 10.0f;
      
      Vector2 text_dim = MeasureTextEx(gs->medium_font, options[i], gs->medium_font.baseSize, 0);
      f32 width = text_dim.x + volume_box.width + padding;
      
      Vec2 text_pos = {WINDOW_WIDTH/2 - width/2, y};
      draw_text(gs->medium_font, options[i], text_pos, color);
      
      f32 yy = (f32)gs->medium_font.baseSize/2 - volume_box.height/2;
      Vec2 volume_box_pos = {WINDOW_WIDTH/2 - width/2 + text_dim.x + padding, y + yy};
      f32 volume = (f32)gs->master_volume/(f32)MAX_MASTER_VOLUME;
      Vec2 dim = {volume_box.width*volume, volume_box.height};
      draw_quad(volume_box_pos, dim, {1,1,1,0.5});
      draw_quad_outline(volume_box_pos, volume_box, 2.0f, WHITE_VEC4);
    }else {
      draw_text_centered(gs->medium_font, options[i], y, color);
    }
    
    y += gs->medium_font.baseSize + y_pad;
  }
  EndDrawing();

}

void do_death_screen(b32 should_update_game = true, char* bottom_text = "Death") {
  Game_State* gs = get_game_state();
  Player* player = get_player();
  
  char* options[2] = {"Restart","Menu"};  
  gs->option_index = Wrap(gs->option_index + get_vertical_navigation_dir(), 0, ArrayCount(options) - 1);
  char* option = options[gs->option_index];

  if(check_confirmation_press()) {
    printf("%s", option);
    if(cstr_equal(option, "Restart")) {
      set_level_to_initial_state();
      change_game_screen(Game_Screen_Game);      
    }
    else if(cstr_equal(option, "Menu")) {
      change_game_screen(Game_Screen_Menu);
    }
  }

  if(should_update_game) update_game();
  
  BeginDrawing();
  draw_game();
  draw_quad({0, 0}, {WINDOW_WIDTH, WINDOW_HEIGHT}, {0.1f, 0.1f, 0.5f, 0.5f});
  
  s32 score_value = gs->score*player->hit_points;
  s32 hs_value = gs->high_score.score*gs->high_score.lives;

  if(score_value > hs_value) {
    gs->high_score = {gs->score, player->hit_points};
    gs->got_high_score = true;
  }
  
  char text[512];
  if(gs->got_high_score) {
    sprintf(text, "New High Score: %dx%d", gs->score, player->hit_points);
  }else {
    sprintf(text, "Score: %dx%d", gs->score, player->hit_points);
  }
  
  draw_text_centered(gs->big_font, text, 200, WHITE_VEC4);
  
  f32 y = 300;
  f32 y_pad = 10.0f;
  Loop(i, ArrayCount(options)) {
    b32 is_selected = (i == gs->option_index);
    
    Vec4 color = WHITE_VEC4;
    if(is_selected) {
      f32 x = GetTime()*12.0f;
      f32 t = (cosf(x) + 1)/2.0f;
      color = vec4_lerp(WHITE_VEC4, YELLOW_VEC4, t);
    }
    
    draw_text_centered(gs->medium_font, options[i], y, color);
    y += gs->medium_font.baseSize + y_pad;
  }
  
  draw_text_centered(gs->medium_font, bottom_text, 500, WHITE_VEC4);
  EndDrawing();
}

void do_win_screen(void) {
  do_death_screen(false, "Level Complete");
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
  asset_catalog_add("audio");
  asset_catalog_add("run_tree/audio");

  // Allocator
  Allocator* allocator = get_allocator();
  
  u32 allocator_size = MB(24);
  u8* allocator_base = (u8*)MemAlloc(allocator_size);
  *allocator = allocator_create(allocator_base, allocator_size);
  
  // Game state init
  Game_State* game_state = get_game_state();
  *game_state = {};
  
  // game screen
  game_state->game_screen = Game_Screen_Menu;
  game_state->level_played_times = 0;
  
  // game object allocation
  game_state->entities      = allocator_alloc_array(allocator, Entity,       MAX_ENTITIES);
  game_state->projectiles   = allocator_alloc_array(allocator, Projectile,   MAX_PROJECTILES);
  game_state->chain_circles = allocator_alloc_array(allocator, Chain_Circle, MAX_CHAIN_CIRCLES);
  game_state->explosions    = allocator_alloc_array(allocator, Explosion,    MAX_EXPLOSIONS);
  game_state->score_dots    = allocator_alloc_array(allocator, Score_Dot,    MAX_SCORE_DOTS);
  game_state->particles     = allocator_alloc_array(allocator, Particle,     MAX_PARTICLES);
  
  // assets
  game_state->chain_circle_texture    = texture_asset_load("chain_circle.png");
  game_state->chain_activator_texture = texture_asset_load("chain_activator.png");
  game_state->laser_bullet_texture    = texture_asset_load("laser_bullet.png");
  
  game_state->songs[0] = music_asset_load("the_rush.mp3");
  game_state->songs[1] = music_asset_load("the_treasure.mp3");
  
  game_state->player_shoot_sound = sound_asset_load("player_shoot.wav");
  game_state->explosion_sound    = sound_asset_load("explosion.wav");
  game_state->laser_shot_sound   = sound_asset_load("laser_shot.wav");
  game_state->score_pickup_sound = sound_asset_load("score_pickup_recent.wav");
  SetSoundVolume(game_state->score_pickup_sound, 0.45f);
  game_state->player_hit_sound   = sound_asset_load("player_hit.wav");
  
  game_state->small_font  = font_asset_load("roboto.ttf", 32);
  game_state->medium_font = font_asset_load("roboto.ttf", 50); 
  game_state->big_font    = font_asset_load("roboto.ttf", 72);
  
  // volume
  game_state->master_volume = MAX_MASTER_VOLUME/2;
  
  // explosion polygon
  s32 point_count = 18;
  game_state->explosion_polygon_index = 0;
  game_state->explosion_timer = timer_start(0.12f);
  game_state->current_explosion_frame_polygon = polygon_alloc(point_count);
  Loop(i, ArrayCount(game_state->explosion_polygons)) {
    game_state->explosion_polygons[i] = polygon_create(point_count, 0.5f, 0.0f);
  }

  // butterfly
  Vec2* top_wing    = game_state->butterfly_top_wing;
  Vec2* bottom_wing = game_state->butterfly_bottom_wing;
  
  top_wing[0] = {0, 0};
  top_wing[1] = {-0.425f, 0.0f};
  top_wing[2] = {-0.525f, -0.2f};
  top_wing[3] = {-0.475f, -0.4f};
  top_wing[4] = {0, -0.1f};

  bottom_wing[0] = {0, 0};
  bottom_wing[1] = {0.0f,  0.1f};
  bottom_wing[2] = {-0.2f, 0.3f};
  bottom_wing[3] = {-0.4f, 0.2f};
  bottom_wing[4] = {-0.35f, 0.0f};
  
  // so that game_draw.cpp has the window dim
  register_draw_dim(WINDOW_WIDTH, WINDOW_HEIGHT);
  
  // inital level state
  //set_level_to_initial_state();
}

void do_game_loop(void) {
  Game_State* gs = get_game_state();
  f32 delta_time = GetFrameTime();
  
  switch(gs->game_screen) {
    case Game_Screen_Menu:    { do_menu_screen();    } break;
    case Game_Screen_Game:    { do_game_screen();    } break;
    case Game_Screen_Credits: { do_credits_screen(); } break;
    case Game_Screen_Paused:  { do_pause_screen();   } break;
    case Game_Screen_Death:   { do_death_screen();   } break;
    case Game_Screen_Win:     { do_win_screen();     } break;
  }
}
