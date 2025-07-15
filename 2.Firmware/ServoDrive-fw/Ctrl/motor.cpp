#include "motor.h"
#include "tim.h"


/**
 * @brief 计算DCE控制器输出
 * @param _inputPos 当前位置反馈值
 * @param _inputVel 当前速度反馈值
 * @return 控制器输出值（扭矩）
 * @details DCE控制器结合了位置和速度控制，包含比例、积分、微分项
 *          具有积分限幅和输出扭矩限制功能
 */
float Motor::CalcDceOutput(float _inputPos, float _inputVel)
{
    // 计算位置和速度误差
    float errorPos = _inputPos - dce.setPointPos;           // 位置误差
    float errorVel = _inputVel - dce.setPointVel;           // 速度误差
    float deltaPos = errorPos - dce.lastError;              // 位置误差变化量（用于微分项）
    dce.lastError = errorPos;                               // 保存当前误差用于下次计算
    
    // 积分项累积并限幅
    dce.integralPos += errorPos;                            // 位置积分累积
    if (dce.integralPos > DCE_INTEGRAL_LIMIT) dce.integralPos = DCE_INTEGRAL_LIMIT;
    else if (dce.integralPos < -DCE_INTEGRAL_LIMIT) dce.integralPos = -DCE_INTEGRAL_LIMIT;
    dce.integralVel += errorVel;                            // 速度积分累积
    if (dce.integralVel > DCE_INTEGRAL_LIMIT) dce.integralVel = DCE_INTEGRAL_LIMIT;
    else if (dce.integralVel < -DCE_INTEGRAL_LIMIT) dce.integralVel = -DCE_INTEGRAL_LIMIT;

    // 计算DCE控制器输出：位置PID + 速度积分
    dce.output = dce.kp * errorPos +                        // 位置比例项
                 dce.ki * dce.integralPos +                 // 位置积分项
                 dce.kv * dce.integralVel +                 // 速度积分项
                 dce.kd * deltaPos;                         // 位置微分项

    // 输出扭矩限制
    if (dce.output > limitTorque) dce.output = limitTorque;
    else if (dce.output < -limitTorque) dce.output = -limitTorque;

    return dce.output;
}


/**
 * @brief 设置电机PWM输出
 * @param _pwm PWM值，范围-1000到1000，正值正转，负值反转
 * @details 使用TIM3的两个通道控制电机正反转
 *          只有在电机使能状态下才输出PWM
 */
void Motor::SetPwm(int16_t _pwm)
{
    if (isEnabled)                                          // 电机使能时才输出PWM
    {
        if (_pwm >= 0)                                      // 正向旋转
        {
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);   // 通道1输出0
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, _pwm > 1000 ? 1000 : _pwm); // 通道2输出PWM
        } else                                              // 反向旋转
        {
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);   // 通道2输出0
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, _pwm < -1000 ? 1000 : -_pwm); // 通道1输出PWM
        }
    } else                                                  // 电机禁用时停止输出
    {
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
    }
}


/**
 * @brief 设置扭矩限制
 * @param _percent 扭矩限制百分比，范围0-1
 * @details 将百分比转换为0-1000的扭矩限制值
 */
void Motor::SetTorqueLimit(float _percent)
{
    // 限制输入范围在0-1之间
    if (_percent > 1)_percent = 1;
    else if (_percent < 0)_percent = 0;

    limitTorque = _percent * 1000;                          // 转换为0-1000范围
}


/**
 * @brief 更新电机速度
 * @details 通过当前角度与上次角度的差值计算速度
 *          需要定期调用以获得准确的速度反馈
 */
void Motor::UpdateVelocity()
{
    velocity = angle - lastAngle;                           // 计算角度变化量作为速度
    lastAngle = angle;                                      // 保存当前角度用于下次计算
}


/**
 * @brief 设置电机使能状态
 * @param _enable true使能电机，false禁用电机
 * @details 禁用时电机不会输出PWM，使能时根据控制器输出驱动电机
 */
void Motor::SetEnable(bool _enable)
{
    isEnabled = _enable;                                    // 设置电机使能标志
}

