#include "motor.h"
#include "tim.h"

/**
 * @brief DCE控制算法计算输出
 * @param _inputPos 当前位置输入
 * @param _inputVel 当前速度输入
 * @return 控制输出值
 * @note DCE = Direct Current Engine，类似PID控制但包含速度环
 *       控制公式：output = Kp*errorPos + Ki*integralPos + Kv*integralVel + Kd*deltaPos
 */
float Motor::CalcDceOutput(float _inputPos, float _inputVel)
{
    // 计算位置误差和速度误差
    float errorPos = _inputPos - dce.setPointPos;  // 位置误差
    float errorVel = _inputVel - dce.setPointVel;  // 速度误差
    float deltaPos = errorPos - dce.lastError;     // 位置误差变化量（微分项）
    dce.lastError = errorPos;
    
    // 位置误差积分，带限幅
    dce.integralPos += errorPos;
    if (dce.integralPos > DCE_INTEGRAL_LIMIT) dce.integralPos = DCE_INTEGRAL_LIMIT;
    else if (dce.integralPos < -DCE_INTEGRAL_LIMIT) dce.integralPos = -DCE_INTEGRAL_LIMIT;
    
    // 速度误差积分，带限幅
    dce.integralVel += errorVel;
    if (dce.integralVel > DCE_INTEGRAL_LIMIT) dce.integralVel = DCE_INTEGRAL_LIMIT;
    else if (dce.integralVel < -DCE_INTEGRAL_LIMIT) dce.integralVel = -DCE_INTEGRAL_LIMIT;

    // DCE控制算法：位置比例 + 位置积分 + 速度积分 + 位置微分
    dce.output = dce.kp * errorPos +           // 位置比例项
                 dce.ki * dce.integralPos +    // 位置积分项
                 dce.kv * dce.integralVel +    // 速度积分项
                 dce.kd * deltaPos;            // 位置微分项

    // 输出限幅，防止超过扭矩限制
    if (dce.output > limitTorque) dce.output = limitTorque;
    else if (dce.output < -limitTorque) dce.output = -limitTorque;

    return dce.output;
}


/**
 * @brief 设置电机PWM输出
 * @param _pwm PWM值（-1000到1000）
 * @note 使用TIM3的两个通道控制电机正反转
 *       正值：CH2输出PWM，CH1为0（正转）
 *       负值：CH1输出PWM，CH2为0（反转）
 *       禁用时：两个通道都输出0
 */
void Motor::SetPwm(int16_t _pwm)
{
    if (isEnabled)  // 电机使能时
    {
        if (_pwm >= 0)  // 正向旋转
        {
            LL_TIM_OC_SetCompareCH1(TIM3, 0);                           // CH1输出0
            LL_TIM_OC_SetCompareCH2(TIM3, _pwm > 1000 ? 1000 : _pwm);   // CH2输出PWM，限制最大值1000
        } 
        else  // 反向旋转
        {
            LL_TIM_OC_SetCompareCH1(TIM3, _pwm < -1000 ? 1000 : -_pwm); // CH1输出PWM，限制最大值1000
            LL_TIM_OC_SetCompareCH2(TIM3, 0);                           // CH2输出0
        }
    } 
    else  // 电机禁用时
    {
        LL_TIM_OC_SetCompareCH1(TIM3, 0);  // 两个通道都输出0
        LL_TIM_OC_SetCompareCH2(TIM3, 0);
    }
}


/**
 * @brief 设置扭矩限制
 * @param _percent 扭矩限制百分比（0.0-1.0）
 * @note 将百分比转换为实际的扭矩限制值（0-1000）
 */
void Motor::SetTorqueLimit(float _percent)
{
    // 限制输入范围在0-1之间
    if (_percent > 1) _percent = 1;
    else if (_percent < 0) _percent = 0;

    limitTorque = _percent * 1000;  // 转换为0-1000的扭矩限制值
}

/**
 * @brief 更新电机速度
 * @note 通过当前角度与上次角度的差值计算速度
 *       需要在固定时间间隔调用以获得准确的速度值
 */
void Motor::UpdateVelocity()
{
    velocity = angle - lastAngle;  // 计算角度变化量作为速度
    lastAngle = angle;             // 保存当前角度为下次计算用
}

/**
 * @brief 设置电机使能状态
 * @param _enable 使能状态（true=使能，false=禁用）
 * @note 禁用时电机将停止输出PWM
 */
void Motor::SetEnable(bool _enable)
{
    isEnabled = _enable;
}

