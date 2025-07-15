/**
 * @file configurations.h
 * @brief 伺服驱动器配置定义
 * @details 定义了板级配置参数结构体和配置状态枚举
 *          包含电机控制参数、I2C通信参数、ADC校准参数等
 */
#ifndef CONFIGURATIONS_H
#define CONFIGURATIONS_H

#ifdef __cplusplus
extern "C" {
#endif
/*---------------------------- C Scope ---------------------------*/
#include <stdbool.h>
#include "stdint-gcc.h"


/**
 * @brief 配置状态枚举
 * @details 用于管理配置参数的保存和恢复状态
 */
typedef enum configStatus_t
{
    CONFIG_RESTORE = 0,     ///< 恢复配置状态（触发系统重启）
    CONFIG_OK,              ///< 配置正常状态
    CONFIG_COMMIT           ///< 提交配置状态（保存到EEPROM）
} configStatus_t;


/**
 * @brief 板级配置参数结构体
 * @details 包含所有需要持久化保存的配置参数
 *          这些参数会保存在Flash模拟的EEPROM中
 */
typedef struct Config_t
{
    configStatus_t configStatus;    ///< 配置状态标志
    uint8_t nodeId;                 ///< I2C节点ID（7位地址）
    float initPos;                  ///< 初始位置（度）
    float toqueLimit;               ///< 扭矩限制（0-1）
    float velocityLimit;            ///< 速度限制
    uint16_t adcValAtAngleMin;      ///< 最小角度对应的ADC值
    uint16_t adcValAtAngleMax;      ///< 最大角度对应的ADC值
    float mechanicalAngleMin;       ///< 机械角度最小值（度）
    float mechanicalAngleMax;       ///< 机械角度最大值（度）
    float dceKp;                    ///< DCE控制器比例增益
    float dceKv;                    ///< DCE控制器速度增益
    float dceKi;                    ///< DCE控制器积分增益
    float dceKd;                    ///< DCE控制器微分增益
    bool enableMotorOnBoot;         ///< 启动时是否使能电机
} BoardConfig_t;

extern BoardConfig_t boardConfig;   ///< 全局板级配置实例


#ifdef __cplusplus
}
/*---------------------------- C++ Scope ---------------------------*/

#include "random_flash_interface.h"  // Flash模拟EEPROM接口
#include "motor.h"                   // 电机控制类


#endif
#endif
