#ifndef FORMAT_H
#define FORMAT_H

#include "../util/datatype.h"

// 无符号整数转字符串
usize u32_to_str(char* buf, u32 val);

// 浮点数转字符串
void f32_to_str(char* buf, float val, u32 decimal_num);

#endif // !FORMAT_H
