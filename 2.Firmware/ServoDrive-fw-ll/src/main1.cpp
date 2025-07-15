#include <cstring>
#include "common_inc.h"
#include "configurations.h"

// 全局变量定义
Motor motor;                // 电机控制对象
BoardConfig_t boardConfig;  // 板级配置参数

/**
 * @brief 主程序入口
 * @note 初始化电机参数、PWM、I2C通信和控制循环
 */
void Main()
{
    // 从EEPROM读取配置数据
    EEPROM eeprom;
    eeprom.get(0, boardConfig);
    
    // 如果配置无效，使用默认设置
    if (boardConfig.configStatus != CONFIG_OK)
    {
        boardConfig = BoardConfig_t{
            .configStatus = CONFIG_OK,
            .nodeId = 12,               // I2C从机地址（7位，必须为偶数）
            .initPos = 90,              // 初始位置（度）
            .toqueLimit = 0.5,          // 扭矩限制（0-1）
            .velocityLimit = 0,         // 速度限制
            .adcValAtAngleMin = 250,    // 最小角度对应的ADC值
            .adcValAtAngleMax = 3000,   // 最大角度对应的ADC值
            .mechanicalAngleMin = 0,    // 机械角度最小值
            .mechanicalAngleMax = 180,  // 机械角度最大值
            .dceKp = 10,               // DCE控制器比例增益
            .dceKv = 0,                // DCE控制器速度增益
            .dceKi = 0,                // DCE控制器积分增益
            .dceKd = 50,               // DCE控制器微分增益
            .enableMotorOnBoot = false  // 启动时是否使能电机
        };
        eeprom.put(0, boardConfig);  // 保存默认配置到EEPROM
    }
    // 从配置中初始化电机参数
    motor.SetTorqueLimit(boardConfig.toqueLimit);           // 设置扭矩限制
    motor.mechanicalAngleMin = boardConfig.mechanicalAngleMin;  // 机械角度范围
    motor.mechanicalAngleMax = boardConfig.mechanicalAngleMax;
    motor.adcValAtAngleMin = boardConfig.adcValAtAngleMin;   // ADC值与角度的映射关系
    motor.adcValAtAngleMax = boardConfig.adcValAtAngleMax;
    motor.dce.kp = boardConfig.dceKp;                       // DCE控制器参数
    motor.dce.ki = boardConfig.dceKi;
    motor.dce.kv = boardConfig.dceKv;
    motor.dce.kd = boardConfig.dceKd;
    motor.dce.setPointPos = boardConfig.initPos;            // 设置初始目标位置
    motor.SetEnable(boardConfig.enableMotorOnBoot);        // 设置启动时的使能状态
    
    // 初始化PWM（TIM3用于电机驱动）
    LL_TIM_EnableCounter(TIM3);                             // 启用定时器计数器
    LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH1);     // 启用通道1（电机正转）
    LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH2);     // 启用通道2（电机反转）
    LL_TIM_OC_SetCompareCH1(TIM3, 0);                      // 初始PWM输出为0
    LL_TIM_OC_SetCompareCH2(TIM3, 0);

    // 初始化I2C从机通信
    MY_I2C1_Init(boardConfig.nodeId);                      // 使用配置的节点ID初始化I2C
    LL_mDelay(10);                                          // 延时等待初始化完成
    
    // 启动200Hz控制循环（TIM14）
    LL_TIM_EnableIT_UPDATE(TIM14);                          // 启用定时器更新中断
    LL_TIM_EnableCounter(TIM14);                            // 启动定时器


    // 主循环：处理配置保存和系统重启
    while (1)
    {
        // 检查是否需要提交配置到EEPROM
        if (boardConfig.configStatus == CONFIG_COMMIT)
        {
            boardConfig.configStatus = CONFIG_OK;  // 重置状态
            eeprom.put(0, boardConfig);             // 保存配置到EEPROM
        } 
        // 检查是否需要恢复配置并重启系统
        else if (boardConfig.configStatus == CONFIG_RESTORE)
        {
            eeprom.put(0, boardConfig);             // 保存配置
            HAL_NVIC_SystemReset();                 // 系统软件重启
        }
        /* 调试用GPIO翻转（已注释）*/
        //LL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);
    }
}


/* Callbacks -------------------------------------------------------*/
// void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* AdcHandle)
// {

// }


/**
 * @brief I2C从机DMA接收完成回调函数
 * @note 处理来自主控制器的各种命令，包括设置/获取角度、速度、扭矩等
 *       命令格式：[命令字节][4字节数据]
 *       响应格式：[命令字节][4字节响应数据]
 */
void I2C_SlaveDMARxCpltCallback()
{
    ErrorStatus state;

    // 将接收到的4字节数据解析为浮点数
    float valF = *((float*) (i2cDataRx + 1));

    // 响应数据的第一个字节回传命令字节
    i2cDataTx[0] = i2cDataRx[0];
    switch (i2cDataRx[0])
    {
        case 0x01:  // 设置目标角度
        {
            motor.dce.setPointPos = valF;  // 设置DCE控制器的目标位置
            // 返回当前角度
            auto* b = (unsigned char*) &(motor.angle);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);
            break;
        }
        case 0x02: // 设置目标速度
        {
            motor.dce.setPointVel = valF;  // 设置DCE控制器的目标速度
            // 返回当前速度
            auto* b = (unsigned char*) &(motor.velocity);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);
            break;
        }
        case 0x03: // 设置扭矩限制
        {
            motor.SetTorqueLimit(valF);    // 设置扭矩限制
            // 返回当前角度
            auto* b = (unsigned char*) &(motor.angle);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);
            break;
        }
        case 0x11: // 获取当前角度
        {
            auto* b = (unsigned char*) &(motor.angle);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);
            break;
        }
        case 0x12: // 获取当前速度
        {
            auto* b = (unsigned char*) &(motor.velocity);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);
            break;
        }
        case 0x21: // 设置设备ID
        {
            boardConfig.nodeId = i2cDataRx[1];         // 设置新的I2C节点ID
            boardConfig.configStatus = CONFIG_COMMIT;  // 标记需要保存配置
            auto* b = (unsigned char*) &(motor.angle);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);
            break;
        }
        case 0x22: // 设置Kp增益
        {
            motor.dce.kp = valF;                       // 更新DCE控制器Kp参数
            boardConfig.dceKp = valF;                  // 更新配置
            boardConfig.configStatus = CONFIG_COMMIT;  // 标记需要保存配置
            auto* b = (unsigned char*) &(motor.angle);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);
            break;
        }
        case 0x23: // 设置Ki增益
        {
            motor.dce.ki = valF;                       // 更新DCE控制器Ki参数
            boardConfig.dceKi = valF;                  // 更新配置
            boardConfig.configStatus = CONFIG_COMMIT;  // 标记需要保存配置
            auto* b = (unsigned char*) &(motor.angle);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);
            break;
        }
        case 0x24: // 设置Kv增益
        {
            motor.dce.kv = valF;                       // 更新DCE控制器Kv参数
            boardConfig.dceKv = valF;                  // 更新配置
            boardConfig.configStatus = CONFIG_COMMIT;  // 标记需要保存配置
            auto* b = (unsigned char*) &(motor.angle);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);
            break;
        }
        case 0x25: // 设置Kd增益
        {
            motor.dce.kd = valF;                       // 更新DCE控制器Kd参数
            boardConfig.dceKd = valF;                  // 更新配置
            boardConfig.configStatus = CONFIG_COMMIT;  // 标记需要保存配置
            auto* b = (unsigned char*) &(motor.angle);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);
            break;
        }
        case 0x26: // 设置扭矩限制
        {
            motor.SetTorqueLimit(valF);                // 设置扭矩限制
            boardConfig.toqueLimit = valF;             // 更新配置
            boardConfig.configStatus = CONFIG_COMMIT;  // 标记需要保存配置
            auto* b = (unsigned char*) &(motor.angle);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);
            break;
        }
        case 0x27: // 设置初始位置
        {
            boardConfig.initPos = valF;                // 设置初始位置
            boardConfig.configStatus = CONFIG_COMMIT;  // 标记需要保存配置
            auto* b = (unsigned char*) &(motor.angle);
            for (int i = 0; i < 4; i++)
                i2cDataTx[i + 1] = *(b + i);
            break;
        }
        case 0xff: // 设置电机使能状态
            motor.SetEnable(i2cDataRx[1] != 0);        // 根据接收到的数据设置使能状态
            break;
        default:
            break;
    }
    // 发送响应数据，重试直到成功
    do
    {
       state = Slave_Transmit(i2cDataTx, 5, 5000);  // 发送5字节响应数据
    } while (state != SUCCESS);
    
    // 如果是设置ID命令，需要重新初始化I2C
    if(i2cDataRx[0] == 0x21)
    {
        Set_ID(boardConfig.nodeId);  // 使用新的ID重新初始化I2C
    }
}

/**
 * @brief TIM14定时器中断回调函数（200Hz控制循环）
 * @note 执行传感器数据读取、角度计算、DCE控制算法和PWM输出
 */
void TIM14_PeriodElapsedCallback()
{
    // 启动ADC转换读取传感器数据
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);  // 启用DMA通道
    LL_ADC_REG_StartConversion(ADC1);              // 启动ADC转换
    
    // 将ADC值转换为机械角度
    // 线性插值：angle = min + (max-min) * (adc-adcMin) / (adcMax-adcMin)
    motor.angle = motor.mechanicalAngleMin +
                    (motor.mechanicalAngleMax - motor.mechanicalAngleMin) *
                    ((float) adcData[0] - (float) motor.adcValAtAngleMin) /
                    ((float) motor.adcValAtAngleMax - (float) motor.adcValAtAngleMin);
    
    // 执行DCE控制算法计算输出
    motor.CalcDceOutput(motor.angle, 0);           // 输入当前角度，速度为0
    
    // 设置PWM输出到电机
    motor.SetPwm((int16_t) motor.dce.output);      // 将控制输出转换为PWM值
}