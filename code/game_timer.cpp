
//
// Timer 
//

enum Timer_State {
  Timer_State_Inactive,
  Timer_State_Active,
  Timer_State_Ended,

  Timer_State_Count
};

struct Timer {
  f64 passed_time, target_time;
  Timer_State state;
};

Timer timer_start(f64 target) {
  Timer r = {};
  r.passed_time = 0.0f;
  r.target_time = target;
  r.state = Timer_State_Active;
  return r;
}

void timer_reset(Timer* timer) {
  timer->passed_time = 0.0f;
  timer->state = Timer_State_Active;
}

b32 timer_step(Timer* timer, f64 seconds) {
  b32 ended_this_frame = false;
  
  timer->passed_time += seconds;
  if(timer->passed_time >= timer->target_time) {
    timer->passed_time = timer->target_time;
    ended_this_frame = true;
    timer->state = Timer_State_Ended;
  }
  return ended_this_frame;
}

b32 timer_step(Timer* timer, f64 seconds, f64 target) {
  timer->target_time = target;
  return timer_step(timer, seconds);
}

f32 timer_procent(Timer timer) {
  f32 r = timer.passed_time/timer.target_time;
  return r;
}

b32 timer_is_inactive(Timer t) { return(t.state == Timer_State_Inactive); }
b32 timer_is_active(Timer t)   { return(t.state == Timer_State_Active); }
b32 timer_ended(Timer t)       { return(t.state == Timer_State_Ended); }

