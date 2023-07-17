//
// General
//
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

#define TITLE             "Arcade Jam"


#define MAX_ENTITIES      256
#define MAX_PROJECTILES   256
#define MAX_CHAIN_CIRCLES 256
#define MAX_SCORE_DOTS    256
#define MAX_EXPLOSIONS    16


//
// Goon 
//
#define GOON_LEADER_COLOR  vec4(0xFF1B68E6)
#define GOON_COLOR         vec4(0xFFE6DC1E)
#define GOON_OUTLINE_COLOR BLACK_VEC4

#define GOON_LEADER_RADIUS 20.0f
#define GOON_RADIUS        15.0f
#define GOON_PADDING       8.0f
#define GOON_MOVE_SPEED    75.0f

//
// Chain circle
//
#define SMALL_CHAIN_CIRCLE  40.0f 
#define BIG_CHAIN_CIRCLE    110.0f
#define MEDIUM_CHAIN_CIRCLE 75.0f

#define CHAIN_CIRCLE_EMERGE_TIME        0.05f
#define MAX_CHAIN_CIRCLE_LIFE_TIME      2.0f
#define CHAIN_CIRCLE_LIFE_PROLONG_TIME  0.4f

//
// Score dot
//
#define SCORE_DOT_RADIUS 6

//
// Colors
//
Vec4 RED_VEC4    = {1, 0, 0, 1};
Vec4 GREEN_VEC4  = {0, 1, 0, 1};
Vec4 BLUE_VEC4   = {0, 0, 1, 1};
Vec4 YELLOW_VEC4 = {1, 1, 0, 1};

Vec4 BLACK_VEC4 = {0, 0, 0, 1};
Vec4 WHITE_VEC4 = {1, 1, 1, 1};