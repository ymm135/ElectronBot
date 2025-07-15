#include "robot.h"
#include "usbd_cdc_if.h"

/**
 * @brief 获取当前活动的ping-pong缓冲区指针
 * @return 返回当前可写入的缓冲区地址
 * @note ping-pong缓冲机制用于避免USB接收数据时的冲突
 */
uint8_t* Robot::GetPingPongBufferPtr()
{
    return &(usbBuffer.rxData[usbBuffer.pingPongIndex][usbBuffer.rxDataOffset]);
}

/**
 * @brief 通过USB CDC发送数据包
 * @param _data 要发送的数据指针
 * @param _len 数据长度
 * @note 使用循环确保数据发送成功
 */
void Robot::SendUsbPacket(uint8_t* _data, uint32_t _len)
{
    uint8_t ret;
    do
    {
        ret = CDC_Transmit_HS(_data, _len);
    } while (ret != USBD_OK);
}

/**
 * @brief 切换ping-pong缓冲区
 * @note 在接收到完整数据包后切换缓冲区，实现双缓冲机制
 */
void Robot::SwitchPingPongBuffer()
{
    usbBuffer.pingPongIndex = (usbBuffer.pingPongIndex == 0 ? 1 : 0);
    usbBuffer.rxDataOffset = 0;
}

/**
 * @brief 获取LCD显示缓冲区指针
 * @return 返回非活动缓冲区地址（用于LCD显示）
 * @note 返回的是另一个缓冲区，避免显示时数据被覆盖
 */
uint8_t* Robot::GetLcdBufferPtr()
{
    return usbBuffer.rxData[usbBuffer.pingPongIndex == 0 ? 1 : 0];
}

/**
 * @brief 等待接收指定长度的USB数据包
 * @param _count 期望接收的数据包长度
 * @note 阻塞等待直到接收到指定长度的数据
 */
void Robot::ReceiveUsbPacketUntilSizeIs(uint32_t _count)
{
    while (usbBuffer.receivedPacketLen != _count);
    usbBuffer.receivedPacketLen = 0;
}


/**
 * @brief 设置舵机目标角度
 * @param _joint 关节状态结构体引用
 * @param _angleSetPoint 目标角度值
 * @note 发送I2C命令0x01设置舵机角度，角度值需在有效范围内
 */
void Robot::UpdateServoAngle(Robot::JointStatus_t &_joint, float _angleSetPoint)
{
    if (_angleSetPoint >= _joint.angleMin && _angleSetPoint <= _joint.angleMax)
    {
        auto* b = (unsigned char*) (&_angleSetPoint);

        i2cTxData[0] = 0x01;  // I2C命令：设置角度
        for (int i = 0; i < 4; i++)
            i2cTxData[i + 1] = *(b + i);  // 将float转换为字节数组

        TransmitAndReceiveI2cPacket(_joint.id);

        _joint.angle = *(float*) (i2cRxData + 1);  // 更新当前角度
    }
}

/**
 * @brief 读取舵机当前角度
 * @param _joint 关节状态结构体引用
 * @note 发送I2C命令0x11获取舵机当前角度
 */
void Robot::UpdateServoAngle(Robot::JointStatus_t &_joint)
{
    auto* b = (unsigned char*) &(_joint.angle);

    i2cTxData[0] = 0x11;  // I2C命令：获取角度

    TransmitAndReceiveI2cPacket(_joint.id);

    _joint.angle = *(float*) (i2cRxData + 1);  // 读取返回的角度值
}

/**
 * @brief 设置关节使能状态
 * @param _joint 关节状态结构体引用
 * @param _enable 使能状态（true=使能，false=禁用）
 * @note 发送I2C命令0xFF控制舵机的使能状态
 */
void Robot::SetJointEnable(Robot::JointStatus_t &_joint, bool _enable)
{
    i2cTxData[0] = 0xff;  // I2C命令：设置使能状态
    i2cTxData[1] = _enable ? 1 : 0;

    TransmitAndReceiveI2cPacket(_joint.id);

    _joint.angle = *(float*) (i2cRxData + 1);
}


/**
 * @brief I2C数据包收发函数
 * @param _id 目标设备的I2C地址
 * @note 先发送5字节命令数据，再接收5字节响应数据
 *       使用循环确保通信成功，超时时间为5ms
 */
void Robot::TransmitAndReceiveI2cPacket(uint8_t _id)
{
    HAL_StatusTypeDef state = HAL_ERROR;
    // 发送I2C数据包，重试直到成功
    do
    {
        state = HAL_I2C_Master_Transmit(motorI2c, _id, i2cTxData, 5, 5);
    } while (state != HAL_OK);
    
    // 接收I2C响应数据，重试直到成功
    do
    {
        state = HAL_I2C_Master_Receive(motorI2c, _id, i2cRxData, 5, 5);
    } while (state != HAL_OK);
}


/**
 * @brief 设置关节扭矩限制
 * @param _joint 关节状态结构体引用
 * @param _percent 扭矩限制百分比（0.0-1.0）
 * @note 发送I2C命令0x26设置扭矩限制，设置后需等待500ms让舵机重置
 */
void Robot::SetJointTorqueLimit(Robot::JointStatus_t &_joint, float _percent)
{
    if (_percent >= 0 && _percent <= 1)
    {
        auto* b = (unsigned char*) (&_percent);

        i2cTxData[0] = 0x26;  // I2C命令：设置扭矩限制
        for (int i = 0; i < 4; i++)
            i2cTxData[i + 1] = *(b + i);  // 将float转换为字节数组

        TransmitAndReceiveI2cPacket(_joint.id);

        _joint.angle = *(float*) (i2cRxData + 1);

        HAL_Delay(500); // 等待舵机重置完成
    }
}


/**
 * @brief 设置关节设备ID
 * @param _joint 关节状态结构体引用
 * @param _id 新的设备ID
 * @note 发送I2C命令0x21设置设备ID，设置后需等待500ms让舵机重置
 */
void Robot::SetJointId(Robot::JointStatus_t &_joint, uint8_t _id)
{
    i2cTxData[0] = 0x21;  // I2C命令：设置设备ID
    i2cTxData[1] = _id;

    TransmitAndReceiveI2cPacket(_joint.id);

    _joint.angle = *(float*) (i2cRxData + 1);

    HAL_Delay(500); // 等待舵机重置完成
}


/**
 * @brief 设置关节初始角度
 * @param _joint 关节状态结构体引用
 * @param _angle 初始角度值
 * @note 发送I2C命令0x27设置初始角度，需要进行角度映射转换
 *       考虑关节是否反向，将模型角度转换为舵机角度
 */
void Robot::SetJointInitAngle(Robot::JointStatus_t &_joint, float _angle)
{
    // 角度映射：将模型角度转换为舵机角度
    float sAngle = _joint.inverted ?
                   (_angle - _joint.modelAngelMin) /
                   (_joint.modelAngelMax - _joint.modelAngelMin) *
                   (_joint.angleMin - _joint.angleMax) + _joint.angleMax :
                   (_angle - _joint.modelAngelMin) /
                   (_joint.modelAngelMax - _joint.modelAngelMin) *
                   (_joint.angleMax - _joint.angleMin) + _joint.angleMin;

    // 检查角度是否在有效范围内
    if (sAngle >= _joint.angleMin && sAngle <= _joint.angleMax)
    {
        auto* b = (unsigned char*) (&_angle);

        i2cTxData[0] = 0x27;  // I2C命令：设置初始角度
        for (int i = 0; i < 4; i++)
            i2cTxData[i + 1] = *(b + i);

        TransmitAndReceiveI2cPacket(_joint.id);

        _joint.angle = *(float*) (i2cRxData + 1);

        HAL_Delay(500); // 等待舵机重置完成
    }
}


/**
 * @brief 设置关节PID控制器的比例增益Kp
 * @param _joint 关节状态结构体引用
 * @param _value Kp值
 * @note 发送I2C命令0x22设置Kp参数
 */
void Robot::SetJointKp(Robot::JointStatus_t &_joint, float _value)
{
    auto* b = (unsigned char*) (&_value);

    i2cTxData[0] = 0x22;  // I2C命令：设置Kp
    for (int i = 0; i < 4; i++)
        i2cTxData[i + 1] = *(b + i);

    TransmitAndReceiveI2cPacket(_joint.id);

    _joint.angle = *(float*) (i2cRxData + 1);

    HAL_Delay(500); // 等待舵机重置完成
}

/**
 * @brief 设置关节PID控制器的积分增益Ki
 * @param _joint 关节状态结构体引用
 * @param _value Ki值
 * @note 发送I2C命令0x23设置Ki参数
 */
void Robot::SetJointKi(Robot::JointStatus_t &_joint, float _value)
{
    auto* b = (unsigned char*) (&_value);

    i2cTxData[0] = 0x23;  // I2C命令：设置Ki
    for (int i = 0; i < 4; i++)
        i2cTxData[i + 1] = *(b + i);

    TransmitAndReceiveI2cPacket(_joint.id);

    _joint.angle = *(float*) (i2cRxData + 1);

    HAL_Delay(500); // 等待舵机重置完成
}

/**
 * @brief 设置关节PID控制器的速度增益Kv
 * @param _joint 关节状态结构体引用
 * @param _value Kv值
 * @note 发送I2C命令0x24设置Kv参数
 */
void Robot::SetJointKv(Robot::JointStatus_t &_joint, float _value)
{
    auto* b = (unsigned char*) (&_value);

    i2cTxData[0] = 0x24;  // I2C命令：设置Kv
    for (int i = 0; i < 4; i++)
        i2cTxData[i + 1] = *(b + i);

    TransmitAndReceiveI2cPacket(_joint.id);

    _joint.angle = *(float*) (i2cRxData + 1);

    HAL_Delay(500); // 等待舵机重置完成
}

/**
 * @brief 设置关节PID控制器的微分增益Kd
 * @param _joint 关节状态结构体引用
 * @param _value Kd值
 * @note 发送I2C命令0x25设置Kd参数
 */
void Robot::SetJointKd(Robot::JointStatus_t &_joint, float _value)
{
    auto* b = (unsigned char*) (&_value);

    i2cTxData[0] = 0x25;  // I2C命令：设置Kd
    for (int i = 0; i < 4; i++)
        i2cTxData[i + 1] = *(b + i);

    TransmitAndReceiveI2cPacket(_joint.id);

    _joint.angle = *(float*) (i2cRxData + 1);

    HAL_Delay(500); // 等待舵机重置完成
}


/**
 * @brief 获取额外数据接收缓冲区指针
 * @return 返回LCD缓冲区后的额外数据区域指针
 * @note LCD数据占用60*240*3字节，额外数据存储在其后
 */
uint8_t* Robot::GetExtraDataRxPtr()
{
    return GetLcdBufferPtr() + 60 * 240 * 3;
}

/**
 * @brief 更新关节角度（读取模式）
 * @param _joint 关节状态结构体引用
 * @note 从舵机读取当前角度，并转换为模型角度
 *       考虑关节反向设置，进行相应的角度映射
 */
void Robot::UpdateJointAngle(Robot::JointStatus_t &_joint)
{
    UpdateServoAngle(_joint);  // 读取舵机当前角度

    // 将舵机角度转换为模型角度
    float jAngle = _joint.inverted ?
                   (_joint.angleMax - _joint.angle) /
                   (_joint.angleMax - _joint.angleMin) *
                   (_joint.modelAngelMax - _joint.modelAngelMin) + _joint.modelAngelMin :
                   (_joint.angle - _joint.angleMin) /
                   (_joint.angleMax - _joint.angleMin) *
                   (_joint.modelAngelMax - _joint.modelAngelMin) + _joint.modelAngelMin;

    _joint.angle = jAngle;
}

/**
 * @brief 更新关节角度（设置模式）
 * @param _joint 关节状态结构体引用
 * @param _angleSetPoint 目标模型角度
 * @note 将模型角度转换为舵机角度并设置，然后读取实际角度
 *       包含双向角度转换：模型角度->舵机角度->模型角度
 */
void Robot::UpdateJointAngle(Robot::JointStatus_t &_joint, float _angleSetPoint)
{
    // 将模型角度转换为舵机角度
    float sAngle = _joint.inverted ?
                   (_angleSetPoint - _joint.modelAngelMin) /
                   (_joint.modelAngelMax - _joint.modelAngelMin) *
                   (_joint.angleMin - _joint.angleMax) + _joint.angleMax :
                   (_angleSetPoint - _joint.modelAngelMin) /
                   (_joint.modelAngelMax - _joint.modelAngelMin) *
                   (_joint.angleMax - _joint.angleMin) + _joint.angleMin;

    UpdateServoAngle(_joint, sAngle);  // 设置舵机角度

    // 将舵机返回的角度转换回模型角度
    float jAngle = _joint.inverted ?
                   (_joint.angleMax - _joint.angle) /
                   (_joint.angleMax - _joint.angleMin) *
                   (_joint.modelAngelMax - _joint.modelAngelMin) + _joint.modelAngelMin :
                   (_joint.angle - _joint.angleMin) /
                   (_joint.angleMax - _joint.angleMin) *
                   (_joint.modelAngelMax - _joint.modelAngelMin) + _joint.modelAngelMin;

    _joint.angle = jAngle;
}
