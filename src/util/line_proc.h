#ifndef LINE_PROC_H
#define LINE_PROC_H

#include "util/datatype.h"

#ifndef GRAD_THR_R
#    define GRAD_THR_R 20.0f                          // 梯度阈值(上升)：用于峰检测，该值越大，越容易剔除噪声，但可能漏检边缘弱峰
#endif

#ifndef GRAD_THR_F
#    define GRAD_THR_F -20.0f                         // 梯度阈值(下降)：用于峰检测，该值越大，越容易剔除噪声，但可能漏检边缘弱峰
#endif

#define FRAME_SIZE 128                              // 数据帧长度
#define INVALID_EDGE 2                              // 边缘无效数据个数
#define GAUSS_TAPS 5                                // 高斯核大小（必须为奇数）
#define DIFF_TAPS 3                                 // 差分核大小（必须为奇数）
#define MAX_PEAKS 5                                 // 最大峰数量（根据实际情况调整，过多可能增加计算负担）
#define VALID_START INVALID_EDGE                    // 有效数据起始索引（前INVALID_EDGE个数据无效）
#define VALID_END (FRAME_SIZE - INVALID_EDGE - 1u)  // 有效数据结束索引（后INVALID_EDGE个数据无效）

typedef struct line_proc_peak_feature
{
    // 峰起始索引
    u16 start_index;
    // 峰终止索引
    u16 end_index;
    // 峰值索引
    u16 peak_idx;
    // 峰值
    f32 peak_value;
} line_proc_peak_feature_t;

typedef struct line_proc_output
{
    // 峰数量（最大5）
    u8 peak_cnt;
    // 峰特征数组
    line_proc_peak_feature_t peaks[MAX_PEAKS];
    // 峰区间二值化结果（128bit）
    u8 binary_bits[FRAME_SIZE / 8];
    // 每条线的中心索引，按检测顺序存放，数量与peak_cnt一致
    u16 line_center_indices[MAX_PEAKS];
} line_proc_output_t;

extern line_proc_output_t g_line_result;

void line_proc_init(void);
void line_proc_set_grad_thr(f32 thr_r, f32 thr_f);
void line_proc_process(const u8* raw_data, line_proc_output_t* output);

// [TOOL] 日志打印线处理结果
void line_proc_log(void);

#endif   // LINE_PROC_H !
