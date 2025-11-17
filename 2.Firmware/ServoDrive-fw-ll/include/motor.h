#ifndef SERVODRIVE_FW_MOTOR_H
#define SERVODRIVE_FW_MOTOR_H

#include <cstdint>

class Motor
{
public:
    Motor()
    {}

    const float DCE_INTEGRAL_LIMIT = 500;

    // 位置-速度双环控制参数集
    struct DCE_t
    {
        float kp;            // 位置比例
        float kv;            // 速度积分系数
        float ki;            // 位置积分系数
        float kd;            // 位置微分系数
        float setPointPos;   // 位置设定值
        float setPointVel;   // 速度设定值
        float integralPos;   // 位置积分累积
        float integralVel;   // 速度积分累积
        float lastError;     // 上一次位置误差
        float output;        // 控制器输出
    };
    DCE_t dce;

    uint16_t adcValAtAngleMin;
    uint16_t adcValAtAngleMax;
    float angle;
    float velocity;
    float mechanicalAngleMin;
    float mechanicalAngleMax;


    void UpdateVelocity();
    void SetEnable(bool _enable);
    void SetTorqueLimit(float _percent);
    float CalcDceOutput(float _inputPos, float _inputVel);
    void SetPwm(int16_t _pwm);

private:
    bool isEnabled;
    float lastAngle;
    float limitAngleMin;
    float limitAngleMax;
    float limitVelocity;
    float limitTorque; // 0~1000，对应 PWM 限幅后的最大扭矩
};

#endif
