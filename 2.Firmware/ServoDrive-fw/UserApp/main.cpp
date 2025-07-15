#include <cstring>
#include "common_inc.h"
#include "configurations.h"

// 电机控制对象
Motor motor;
// 板级配置参数
BoardConfig_t boardConfig;


/* Default Entry -------------------------------------------------------*/
/**
 * @brief 主程序入口函数
 * @details 初始化电机参数、PWM、I2C通信和控制循环
 *          从EEPROM读取配置参数，如果配置无效则使用默认值
 *          启动200Hz控制循环进行电机位置控制
 */
void Main()
{
    // 从Flash读取配置数据
    EEPROM eeprom;
    eeprom.get(0, boardConfig);
    if (boardConfig.configStatus != CONFIG_OK) // 使用默认设置
    {
        boardConfig = BoardConfig_t{
            .configStatus = CONFIG_OK,        // 配置状态标志
            .nodeId = 12,                     // I2C节点ID（7位地址，必须为偶数）
            .initPos = 90,                    // 初始位置（度）
            .toqueLimit =  0.5,               // 扭矩限制（0-1）
            .velocityLimit=0,                 // 速度限制
            .adcValAtAngleMin=250,            // 最小角度对应的ADC值
            .adcValAtAngleMax=3000,           // 最大角度对应的ADC值
            .mechanicalAngleMin=0,            // 机械角度最小值（度）
            .mechanicalAngleMax=180,          // 机械角度最大值（度）
            .dceKp = 10,                      // DCE控制器比例增益
            .dceKv = 0,                       // DCE控制器速度增益
            .dceKi = 0,                       // DCE控制器积分增益
            .dceKd = 50,                      // DCE控制器微分增益
            .enableMotorOnBoot=false          // 启动时是否使能电机
        };
        eeprom.put(0, boardConfig);
    }
    // 设置电机参数
    motor.SetTorqueLimit(boardConfig.toqueLimit);           // 设置扭矩限制
    motor.mechanicalAngleMin = boardConfig.mechanicalAngleMin; // 设置机械角度范围
    motor.mechanicalAngleMax = boardConfig.mechanicalAngleMax;
    motor.adcValAtAngleMin = boardConfig.adcValAtAngleMin;   // 设置ADC值与角度的映射关系
    motor.adcValAtAngleMax = boardConfig.adcValAtAngleMax;
    motor.dce.kp = boardConfig.dceKp;                       // 设置DCE控制器参数
    motor.dce.ki = boardConfig.dceKi;
    motor.dce.kv = boardConfig.dceKv;
    motor.dce.kd = boardConfig.dceKd;
    motor.dce.setPointPos = boardConfig.initPos;            // 设置初始目标位置
    motor.SetEnable(boardConfig.enableMotorOnBoot);        // 设置电机使能状态

    // 初始化PWM输出
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);               // 启动TIM3通道1 PWM
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);               // 启动TIM3通道2 PWM
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);        // 初始占空比设为0
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);


    // 启动I2C通信
    MY_I2C1_Init(boardConfig.nodeId);                       // 初始化I2C从机，设置节点ID
    HAL_Delay(10);
    HAL_I2C_Slave_Receive_IT(&hi2c1, (uint8_t*) i2cDataRx, 5); // 启动I2C中断接收

    // 启动200Hz控制循环
    HAL_TIM_Base_Start_IT(&htim14);                         // 启动TIM14定时器中断


    // 主循环：处理配置保存和恢复
    while (1)
    {
        if (boardConfig.configStatus == CONFIG_COMMIT)          // 提交配置到EEPROM
        {
            boardConfig.configStatus = CONFIG_OK;
            eeprom.put(0, boardConfig);
        } else if (boardConfig.configStatus == CONFIG_RESTORE)  // 恢复配置并重启系统
        {
            eeprom.put(0, boardConfig);
            HAL_NVIC_SystemReset();
        }

        HAL_Delay(10);
    }
}


/* Callbacks -------------------------------------------------------*/
/**
 * @brief ADC转换完成回调函数
 * @param AdcHandle ADC句柄
 * @details 当前为空实现，ADC数据在控制循环中直接读取
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* AdcHandle)
{

}


/**
 * @brief I2C从机接收完成回调函数（命令处理器）
 * @param hi2c I2C句柄
 * @details 处理来自主控的各种I2C命令：
 *          0x01-0x03: 设置角度、速度、扭矩
 *          0x11-0x12: 获取角度、速度
 *          0x21-0x27: 设置ID、PID参数、扭矩限制、初始位置
 *          0xff: 使能/禁用电机
 */
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef* hi2c)
{
    HAL_StatusTypeDef state = HAL_ERROR;

    float valF = *((float*) (i2cDataRx + 1));               // 将接收数据转换为浮点数

    i2cDataTx[0] = i2cDataRx[0];                            // 回传命令字
    switch (i2cDataRx[0])
    {
        case 0x01:  // 设置目标角度
        {
            motor.dce.setPointPos = valF;
            auto* b = (unsigned char*) &(motor.angle);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);                // 返回当前角度
            break;
        }
        case 0x02: // 设置目标速度
        {
            motor.dce.setPointVel = valF;
            auto* b = (unsigned char*) &(motor.velocity);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);                // 返回当前速度
            break;
        }
        case 0x03: // 设置扭矩限制
        {
            motor.SetTorqueLimit(valF);
            auto* b = (unsigned char*) &(motor.angle);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);                // 返回当前角度
            break;
        }
        case 0x11: // 获取当前角度
        {
            auto* b = (unsigned char*) &(motor.angle);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);                // 返回当前角度（浮点数转字节）
            break;
        }
        case 0x12: // 获取当前速度
        {
            auto* b = (unsigned char*) &(motor.velocity);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);                // 返回当前速度（浮点数转字节）
            break;
        }
        case 0x21: // 设置节点ID
        {
            boardConfig.nodeId = i2cDataRx[1];
            boardConfig.configStatus = CONFIG_COMMIT;              // 标记需要保存配置
            auto* b = (unsigned char*) &(motor.angle);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);                // 返回当前角度
            break;
        }
        case 0x22: // 设置比例增益Kp
        {
            motor.dce.kp = valF;
            boardConfig.dceKp = valF;
            boardConfig.configStatus = CONFIG_COMMIT;              // 标记需要保存配置
            auto* b = (unsigned char*) &(motor.angle);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);                // 返回当前角度
            break;
        }
        case 0x23: // 设置积分增益Ki
        {
            motor.dce.ki = valF;
            boardConfig.dceKi = valF;
            boardConfig.configStatus = CONFIG_COMMIT;              // 标记需要保存配置
            auto* b = (unsigned char*) &(motor.angle);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);                // 返回当前角度
            break;
        }
        case 0x24: // 设置速度增益Kv
        {
            motor.dce.kv = valF;
            boardConfig.dceKv = valF;
            boardConfig.configStatus = CONFIG_COMMIT;              // 标记需要保存配置
            auto* b = (unsigned char*) &(motor.angle);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);                // 返回当前角度
            break;
        }
        case 0x25: // 设置微分增益Kd
        {
            motor.dce.kd = valF;
            boardConfig.dceKd = valF;
            boardConfig.configStatus = CONFIG_COMMIT;              // 标记需要保存配置
            auto* b = (unsigned char*) &(motor.angle);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);                // 返回当前角度
            break;
        }
        case 0x26: // 设置扭矩限制并保存
        {
            motor.SetTorqueLimit(valF);
            boardConfig.toqueLimit = valF;
            boardConfig.configStatus = CONFIG_COMMIT;              // 标记需要保存配置
            auto* b = (unsigned char*) &(motor.angle);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);                // 返回当前角度
            break;
        }
        case 0x27: // 设置初始位置
        {
            boardConfig.initPos = valF;
            boardConfig.configStatus = CONFIG_COMMIT;              // 标记需要保存配置
            auto* b = (unsigned char*) &(motor.angle);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);                // 返回当前角度
            break;
        }
        case 0xff: // 使能/禁用电机
            motor.SetEnable(i2cDataRx[1] != 0);                    // 非0值使能电机，0值禁用电机
            break;
        default:
            break;
    }

    // 发送响应数据
    do
    {
        state = HAL_I2C_Slave_Transmit(&hi2c1, (uint8_t*) i2cDataTx, 5, 10000);
    } while (state != HAL_OK);

    // 准备接收下一个命令
    do
    {
        state = HAL_I2C_Slave_Receive_IT(&hi2c1, (uint8_t*) i2cDataRx, 5);
    } while (state != HAL_OK);
}


/**
 * @brief 定时器周期回调函数（200Hz控制循环）
 * @param htim 定时器句柄
 * @details 每5ms执行一次，实现电机位置控制：
 *          1. 读取ADC传感器数据
 *          2. 将ADC值转换为角度
 *          3. 计算DCE控制输出
 *          4. 设置PWM输出
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
    if (htim->Instance == TIM14)
    {
        // 读取传感器数据
        HAL_ADC_Start_DMA(&hadc, (uint32_t*) adcData, 1);

        // 将ADC值线性映射为角度值
        motor.angle = motor.mechanicalAngleMin +
                      (motor.mechanicalAngleMax - motor.mechanicalAngleMin) *
                      ((float) adcData[0] - (float) motor.adcValAtAngleMin) /
                      ((float) motor.adcValAtAngleMax - (float) motor.adcValAtAngleMin);

        // 计算DCE控制输出
        motor.CalcDceOutput(motor.angle, 0);
        // 设置PWM输出到电机
        motor.SetPwm((int16_t) motor.dce.output);
    }
}