#include "line_proc.h"
#include "arm_math.h"
#include "datatype.h"
#include "log/logger.h"
#include <string.h>
#include "cmsis_os2.h"

line_proc_output_t g_line_result;
f32 g_grad_thr_r = GRAD_THR_R;
f32 g_grad_thr_f = GRAD_THR_F;

// 5点高斯核（σ≈1.0）：用于抑制高频噪声，尽量保持边缘形状
static const f32 gauss_coeffs[GAUSS_TAPS] = {
    0.06136f,
    0.24477f,
    0.38774f,
    0.24477f,
    0.06136f,
};

// 一维差分核：计算一阶梯度，突出灰度突变位置
static const f32 diff_coeffs[DIFF_TAPS] = {
    -1.0f,
    0.0f,
    1.0f,
};

// FIR实例：分别用于高斯平滑和差分梯度计算
static arm_fir_instance_f32 fir_gauss;
static arm_fir_instance_f32 fir_diff;

// FIR状态缓冲区，长度要求为 block_size + taps - 1
static f32 fir_state_gauss[FRAME_SIZE + GAUSS_TAPS - 1];
static f32 fir_state_diff[FRAME_SIZE + DIFF_TAPS - 1];

// 中间数据缓存
static f32 input_f32[FRAME_SIZE];        // 原始输入转换为f32
static f32 fir_gauss_data[FRAME_SIZE];   // 高斯滤波输出
static f32 fir_diff_data[FRAME_SIZE];    // 差分滤波输出（梯度）

// 将u8原始数据转换为f32缓存
static void transfer_u8_to_f32(const u8* raw_data)
{
    for (u16 index = 0; index < FRAME_SIZE; index++) {
        input_f32[index] = (f32)raw_data[index];
    }
}

// 查找并提取一个有效峰（从当前index开始，成功时填充peak并返回true）
static bool line_proc_find_single_peak(u16* index, line_proc_peak_feature_t* peak)
{
    // 参数检查：避免空指针导致越界访问
    if (index == NULL || peak == NULL) {
        return false;
    }

    u16 cur_idx = *index;
    u16 grad_max_idx, grad_min_idx;
    f32 grad_max, grad_min;

    // 1) 定位候选上升段起点：跳过低于阈值的梯度
    while (cur_idx <= VALID_END && fir_diff_data[cur_idx] < g_grad_thr_r) {
        cur_idx++;
    }
    if (cur_idx > VALID_END) {
        *index = cur_idx;
        return false;
    }

    // 2) 扫描上升段，提取上升最快点与最大正梯度
    grad_max     = fir_diff_data[cur_idx];
    grad_max_idx = cur_idx;
    while (cur_idx <= VALID_END) {
        if (fir_diff_data[cur_idx] < 0.0f) {
            break;
        }
        if (fir_diff_data[cur_idx] > grad_max) {
            grad_max     = fir_diff_data[cur_idx];
            grad_max_idx = cur_idx;
        }
        cur_idx++;
    }

    // 3) 等待下降段开始：跳过未达到负阈值的区间
    while (cur_idx <= VALID_END && fir_diff_data[cur_idx] > g_grad_thr_f) {
        cur_idx++;
    }
    if (cur_idx > VALID_END) {
        *index = cur_idx;
        return false;
    }

    // 4) 扫描下降段，提取下降最快点与最小负梯度
    grad_min     = fir_diff_data[cur_idx];
    grad_min_idx = cur_idx;
    while (cur_idx <= VALID_END) {
        if (fir_diff_data[cur_idx] > 0.0f) {
            break;
        }
        if (fir_diff_data[cur_idx] < grad_min) {
            grad_min     = fir_diff_data[cur_idx];
            grad_min_idx = cur_idx;
        }
        cur_idx++;
    }

    // 5) 峰有效性筛选：剔除谷，梯度幅值，宽度必须满足要求
    if (grad_min_idx <= grad_max_idx) {
        *index = cur_idx;
        return false;
    }
    if (grad_max < g_grad_thr_r || grad_min > g_grad_thr_f) {
        *index = cur_idx;
        return false;
    }

    // 6) 峰特征提取：峰值为上升段内的最大值，峰宽为上升段起点到下降段终点
    f32 peak_value = 0.0f;
    u32 peak_idx   = 0;
    u16 peak_len   = grad_min_idx - grad_max_idx + 1;
    arm_max_f32(&fir_gauss_data[grad_max_idx], peak_len, &peak_value, &peak_idx);
    peak->start_index = grad_max_idx;
    peak->end_index   = grad_min_idx;
    peak->peak_value  = peak_value;
    peak->peak_idx    = peak->start_index + peak_idx;

    *index = cur_idx;
    return true;
}

void line_proc_init(void)
{
    arm_fir_init_f32(&fir_gauss, GAUSS_TAPS, gauss_coeffs, fir_state_gauss, FRAME_SIZE);
    arm_fir_init_f32(&fir_diff, DIFF_TAPS, diff_coeffs, fir_state_diff, FRAME_SIZE);
}

void line_proc_set_grad_thr(f32 thr_r, f32 thr_f)
{
    g_grad_thr_r = thr_r;
    g_grad_thr_f = thr_f;
}

void line_proc_process(const u8* raw_data, line_proc_output_t* output)
{
    // 参数检查
    if (raw_data == NULL || output == NULL) {
        return;
    }
    memset(output, 0, sizeof(*output));

    // 输入转f32
    transfer_u8_to_f32(raw_data);

    // 高斯滤波
    arm_fir_f32(&fir_gauss, input_f32, fir_gauss_data, FRAME_SIZE);

    // 差分卷积计算梯度
    arm_fir_f32(&fir_diff, fir_gauss_data, fir_diff_data, FRAME_SIZE);
    // 打印梯度原始数据
    #if LINE_PROC_DIFF
    trace_raw(RTT_CTRL_TEXT_BRIGHT_YELLOW "tick:%u, diff data:\n", osKernelGetTickCount());
    for (u32 i = 0; i < FRAME_SIZE; i++) {
        trace_raw("%d ", (i32)fir_diff_data[i]);
    }
    trace_raw(_RLOG_COLOR_RST "\n");
    #endif

    // 扫描并提取多个峰：每次循环尝试提取一个峰，直到到达边界或达到峰数量上限
    u16 index = VALID_START;
    while (index <= VALID_END && output->peak_cnt < MAX_PEAKS) {
        line_proc_peak_feature_t* peak = &output->peaks[output->peak_cnt];
        if (line_proc_find_single_peak(&index, peak)) {
            output->peak_cnt++;
        }
    }

    // 根据提取的峰特征生成二值化结果和线中心索引
    for (u8 peak_index = 0; peak_index < output->peak_cnt; peak_index++) {
        u16 start_index = output->peaks[peak_index].start_index;
        u16 end_index   = output->peaks[peak_index].end_index;

        // 中心索引采用区间中点（向下取整）
        output->line_center_indices[peak_index] = (u16)((start_index + end_index) / 2u);

        for (u16 bit_index = start_index; bit_index <= end_index; bit_index++) {
            output->binary_bits[bit_index / 8u] |= (u8)(1u << (bit_index % 8u));
        }
    }
}

/**
 * @brief [TOOL] 日志打印线处理结果
 * 
 */
void line_proc_log(void)
{
    trace_raw(RTT_CTRL_TEXT_BRIGHT_BLUE "tick:%u, peak_cnt:%u\n", osKernelGetTickCount(), g_line_result.peak_cnt);
    for (u8 i = 0; i < g_line_result.peak_cnt && i < MAX_PEAKS; i++) {
        trace_raw("peak[%u] start=%u end=%u\n",
                 i,
                 g_line_result.peaks[i].start_index,
                 g_line_result.peaks[i].end_index);
    }

    trace_raw("binary: ");
    for (u16 pixel = 0; pixel < FRAME_SIZE; pixel++) {
        u8 bit_val = (g_line_result.binary_bits[pixel / 8u] >> (pixel % 8u)) & 1u;
        trace_raw("%u", bit_val);
    }
    trace_raw(_RLOG_COLOR_RST "\n");
}
