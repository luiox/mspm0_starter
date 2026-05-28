#ifndef ROBOT_SYS_PID_H
#define ROBOT_SYS_PID_H

#include "../util/datatype.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum pid_mode_enum
{
    PID_MODE_POSITION  = 0,
    PID_MODE_INCREMENT = 1,
} pid_mode_t;

typedef struct pid_config
{
    f32        kp;
    f32        ki;
    f32        kd;
    f32        max_output;
    f32        max_integral;
    f32        integral_band;
    pid_mode_t mode;
} pid_config_t;

typedef struct pid_state
{
    f32 target;
    f32 output;
    f32 p_out;
    f32 i_out;
    f32 d_out;
    f32 error;
    f32 prev_error;
    f32 prev_prev_error;
    f32 integral;
} pid_state_t;

typedef struct pid
{
    pid_config_t config;
    pid_state_t  state;
} pid_t;

void pid_init(pid_t* self, pid_mode_t mode);
void pid_reset(pid_t* self);
void pid_set_target(pid_t* self, f32 target);
void pid_set_params(pid_t* self, f32 kp, f32 ki, f32 kd);
void pid_set_limits(pid_t* self, f32 max_output, f32 max_integral);
void pid_set_integral_band(pid_t* self, f32 band);
f32  pid_calculate(pid_t* self, f32 feedback, f32 delta_t_s);

#ifdef __cplusplus
}
#endif

#endif
