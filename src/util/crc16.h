/**
 * @file crc16.h
 * @brief CRC16 Modbus 校验计算（查表法）
 * @note  可用于 Modbus-RTU 等协议的 CRC16 校验
 * @version 0.1
 * @date 2026-05-08
 */
#ifndef UTIL_CRC16_H
#define UTIL_CRC16_H

#include "../util/datatype.h"

/**
 * @brief 计算 Modbus CRC16
 * @param p_data 数据指针
 * @param len    数据长度
 * @return CRC16 值（低字节在前）
 */
u16 crc16_modbus(const u8 *p_data, u16 len);

#endif /* UTIL_CRC16_H */
