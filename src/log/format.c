#include "format.h"
#include "rtt_logger.h"

// ============================================================
// 运行时 tag 栈实现（定义在唯一的 .c 文件中）
// ============================================================
static const char* s_tag_stack[RLOG_TAG_STACK_DEPTH];
static int         s_tag_stack_ptr = 0;  // 指向下一个空闲槽，0 表示栈空

void rlog_push_tag(const char* tag)
{
    if (s_tag_stack_ptr < RLOG_TAG_STACK_DEPTH) {
        s_tag_stack[s_tag_stack_ptr++] = tag;
    }
}

void rlog_pop_tag(void)
{
    if (s_tag_stack_ptr > 0) {
        s_tag_stack_ptr--;
    }
}

const char* rlog_get_effective_tag(void)
{
    return s_tag_stack_ptr > 0 ? s_tag_stack[s_tag_stack_ptr - 1] : NULL;
}

// 辅助函数：将无符号整数转换为字符串，返回写入的字符长度
// buf: 目标缓冲区
// val: 待转换的无符号整数
// return: 转换后的字符长度
usize u32_to_str(char *buf, u32 val) {
    char tmp[12]; // 32位整数最大10位，预留空间
    int i = 0;
    usize len = 0;

    // 特殊情况处理：0
    if (val == 0) {
        *buf = '0';
        return 1;
    }

    // 逆序生成数字字符
    while (val > 0) {
        tmp[i++] = (val % 10) + '0';
        val /= 10;
    }

    // 翻转回正序写入buf
    len = i;
    while (i > 0) {
        *buf++ = tmp[--i];
    }

    return len;
}

// 主函数：浮点转字符串
// buf: 目标缓冲区（需保证足够空间，建议至少20字节）
// val: 浮点数值
// decimal_num: 保留小数位数
void f32_to_str(char *buf, float val, u32 decimal_num) {
    char *p = buf;
    u32 int_part;
    u32 frac_part;
    u32 pow_ten = 1;
    int i;

    // 1. 处理符号
    if (val < 0) {
        *p++ = '-';
        val = -val; // 取绝对值进行后续处理
    }

    // 2. 分离整数部分
    // 强制转换直接截断小数
    int_part = (u32)val;

    // 3. 分离小数部分
    float frac_val = val - (float)int_part;

    // 4. 计算10的N次方
    // 这是一个简单的整数幂运算，仅用于小数位数较少的情况
    for (i = 0; i < (int)decimal_num; i++) {
        pow_ten *= 10;
    }

    // 5. 提取小数部分的整数值 (截断法)
    // 例如：0.5678 * 100 = 56.78 -> int转换为 56
    // 注意：此处存在浮点精度误差，例如可能得到 55 或 56，符合“无需特别精准”的要求
    if (pow_ten > 0) {
        frac_part = (u32)(frac_val * pow_ten);
    } else {
        frac_part = 0;
    }

    // 6. 转换整数部分
    p += u32_to_str(p, int_part);

    // 7. 处理小数部分
    if (decimal_num > 0) {
        *p++ = '.'; // 添加小数点

        // 关键点：处理小数部分的前导零
        // 例如：整数部分是1，小数部分计算结果为 5 (期望是 1.05)
        // 如果直接转换 "5"，结果会是 "1.5"，漏掉了一个0。
        // 我们需要根据位数补0。
        u32 temp_pow = pow_ten / 10;
        while (temp_pow > 0 && frac_part < temp_pow) {
            *p++ = '0';
            temp_pow /= 10;
        }

        // 转换小数部分的数字
        // 如果小数部分为0，也要确保打印出 ".00" 这样的格式
        if (frac_part > 0) {
            p += u32_to_str(p, frac_part);
        } else {
            // 如果计算出的小数部分是0，补齐所需的0个数
            for (i = 0; i < (int)decimal_num; i++) {
                *p++ = '0';
            }
        }
    }

    // 8. 结束符
    *p = '\0';
}
