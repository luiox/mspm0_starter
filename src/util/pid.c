#include "pid.h"
#include <math.h>

static f32 pid_limit(f32 value, f32 limit)
{
    if (limit <= 0.0f) {
        return value;
    }
    if (value > limit) {
        return limit;
    }
    if (value < -limit) {
        return -limit;
    }
    return value;
}

static void pid_update_errors(pid_state_t* state, f32 error)
{
    state->prev_prev_error = state->prev_error;
    state->prev_error      = state->error;
    state->error           = error;
}

static f32 pid_calculate_position(pid_t* self, f32 feedback, f32 delta_t_s)
{
    f32 error = self->state.target - feedback;
    f32 output;
    f32 p_out;
    f32 i_out;
    f32 d_out;
    f32 dt;

    dt = delta_t_s > 0.0f ? delta_t_s : 1e-3f;

    pid_update_errors(&self->state, error);

    p_out = self->config.kp * self->state.error;

    if (self->config.ki != 0.0f) {
        if (self->config.integral_band <= 0.0f ||
            fabsf(self->state.error) <= self->config.integral_band) {
            self->state.integral += self->state.error * dt;
        }
    }
    if (self->config.max_integral > 0.0f) {
        self->state.integral = pid_limit(self->state.integral, self->config.max_integral);
    }
    i_out = self->config.ki * self->state.integral;

    d_out = self->config.kd * (self->state.error - self->state.prev_error) / dt;

    output = p_out + i_out + d_out;

    if (self->config.max_output > 0.0f) {
        if (fabsf(output) > self->config.max_output && self->config.ki != 0.0f) {
            self->state.integral -= self->state.error * dt;
            i_out  = self->config.ki * self->state.integral;
            output = p_out + i_out + d_out;
        }
        output = pid_limit(output, self->config.max_output);
    }

    self->state.p_out  = p_out;
    self->state.i_out  = i_out;
    self->state.d_out  = d_out;
    self->state.output = output;
    return output;
}

static f32 pid_calculate_increment(pid_t* self, f32 feedback, f32 delta_t_s)
{
    f32 error = self->state.target - feedback;
    f32 delta;
    f32 p_out;
    f32 i_out;
    f32 d_out;
    f32 output;
    f32 dt;

    dt = delta_t_s > 0.0f ? delta_t_s : 1e-3f;

    pid_update_errors(&self->state, error);

    p_out = self->config.kp * (self->state.error - self->state.prev_error);

    i_out = 0.0f;
    if (self->config.ki != 0.0f) {
        if (self->config.integral_band <= 0.0f ||
            fabsf(self->state.error) <= self->config.integral_band) {
            if (self->config.integral_band > 0.0f) {
                i_out = self->config.ki * self->state.error * dt *
                        fabsf(self->state.error / self->config.integral_band);
            }
            else {
                i_out = self->config.ki * self->state.error * dt;
            }
        }
    }

    d_out = self->config.kd *
            (self->state.error - 2.0f * self->state.prev_error + self->state.prev_prev_error) / dt;

    delta  = p_out + i_out + d_out;
    output = self->state.output + delta;

    if (self->config.max_output > 0.0f) {
        if (fabsf(output) > self->config.max_output && self->config.ki != 0.0f) {
            delta -= i_out;
            output = self->state.output + delta;
        }
        output = pid_limit(output, self->config.max_output);
    }

    self->state.p_out  = p_out;
    self->state.i_out  = i_out;
    self->state.d_out  = d_out;
    self->state.output = output;
    return output;
}

void pid_init(pid_t* self, pid_mode_t mode)
{
    if (self == NULL) {
        return;
    }
    self->config.kp            = 0.0f;
    self->config.ki            = 0.0f;
    self->config.kd            = 0.0f;
    self->config.max_output    = 0.0f;
    self->config.max_integral  = 0.0f;
    self->config.integral_band = 0.0f;
    self->config.mode          = mode;
    pid_reset(self);
}

void pid_reset(pid_t* self)
{
    if (self == NULL) {
        return;
    }
    self->state.target          = 0.0f;
    self->state.output          = 0.0f;
    self->state.p_out           = 0.0f;
    self->state.i_out           = 0.0f;
    self->state.d_out           = 0.0f;
    self->state.error           = 0.0f;
    self->state.prev_error      = 0.0f;
    self->state.prev_prev_error = 0.0f;
    self->state.integral        = 0.0f;
}

void pid_set_target(pid_t* self, f32 target)
{
    if (self == NULL) {
        return;
    }
    self->state.target = target;
}

void pid_set_params(pid_t* self, f32 kp, f32 ki, f32 kd)
{
    if (self == NULL) {
        return;
    }
    self->config.kp = kp;
    self->config.ki = ki;
    self->config.kd = kd;
}

void pid_set_limits(pid_t* self, f32 max_output, f32 max_integral)
{
    if (self == NULL) {
        return;
    }
    self->config.max_output   = max_output;
    self->config.max_integral = max_integral;
}

void pid_set_integral_band(pid_t* self, f32 band)
{
    if (self == NULL) {
        return;
    }
    self->config.integral_band = band;
}

f32 pid_calculate(pid_t* self, f32 feedback, f32 delta_t_s)
{
    if (self == NULL) {
        return 0.0f;
    }
    if (self->config.mode == PID_MODE_INCREMENT) {
        return pid_calculate_increment(self, feedback, delta_t_s);
    }
    return pid_calculate_position(self, feedback, delta_t_s);
}
