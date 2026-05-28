/**
 * @file datatype.h
 * @author canrad (1517807724@qq.com)
 * @brief 基础类型的定义
 * 位，字节，字节序相关的操作
 * @version 0.2
 * @date 2025-07-21
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef DATATYPE_H
#define DATATYPE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// 整数
typedef uint8_t      u8;
typedef uint16_t     u16;
typedef uint32_t     u32;
typedef int8_t       i8;
typedef int16_t      i16;
typedef int32_t      i32;
#ifdef HAS_INT64
typedef uint64_t     u64;
typedef int64_t      i64;
#endif
// 浮点数
typedef float        f32;
typedef double       f64;
// size
typedef size_t          usize;

// 对于需要精准控制高低位的情况下使用下面的宏
// 其中bits是一个整数类型的变量，一般是u8、u16、u32等
// n是位的索引，从0开始
// val是要设置的值，0或1
// 取一个位上的值
#define bits_get(bits, n) (((bits) >> (n)) & 0x01)
// 设置一个位上的值
#define bits_set(bits, n, val) ((bits) = ((bits) & ~(1 << (n))) | ((val) << (n)))
// 反转一个位上的值
#define bits_toggle(bits, n) ((bits) ^= (1 << (n)))
// 获取第n位的掩码
#define bits_mask(n) (1U << (n))
// 获取低n位的掩码
#define bits_mask_low(n) ((1U << (n)) - 1)
// 检查某一位是否为1
#define bits_check_bit(bits, n) (((bits) & (1U << (n))) != 0)

// 获取数组元素个数
#define array_size(arr) (sizeof(arr) / sizeof((arr)[0]))

// 标记未使用的参数
// 例: unused_param(a);
#define unused_param(param) (void)(param)

#endif   // !DATATYPE_H
