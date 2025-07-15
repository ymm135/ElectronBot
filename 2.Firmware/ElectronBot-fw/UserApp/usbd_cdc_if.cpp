#include "usbd_cdc_if.h"
#include "robot.h"

// 外部变量声明
extern Robot electron;                  // 机器人控制对象
extern USBD_HandleTypeDef hUsbDeviceHS; // USB设备句柄

// USB CDC接口函数声明
static int8_t CDC_Init_HS();
static int8_t CDC_DeInit_HS();
static int8_t CDC_Control_HS(uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Receive_HS(uint8_t* pbuf, uint32_t* Len);
static int8_t CDC_TransmitCplt_HS(uint8_t* pbuf, uint32_t* Len, uint8_t epnum);

/**
 * @brief USB CDC接口函数表
 * @note 定义USB CDC类的回调函数，用于处理初始化、数据收发等操作
 */
USBD_CDC_ItfTypeDef USBD_Interface_fops_HS =
    {
        CDC_Init_HS,            // 初始化函数
        CDC_DeInit_HS,          // 反初始化函数
        CDC_Control_HS,         // 控制命令处理函数
        CDC_Receive_HS,         // 数据接收函数
        CDC_TransmitCplt_HS     // 发送完成回调函数
    };


/**
 * @brief USB CDC高速接口初始化
 * @return USBD_OK 初始化成功
 * @note 设置USB发送和接收缓冲区，使用ping-pong缓冲机制
 */
static int8_t CDC_Init_HS()
{
    // 设置USB发送缓冲区
    USBD_CDC_SetTxBuffer(&hUsbDeviceHS, electron.usbBuffer.extraDataTx, 0);
    // 设置USB接收缓冲区（使用ping-pong缓冲区）
    USBD_CDC_SetRxBuffer(&hUsbDeviceHS, electron.usbBuffer.rxData[electron.usbBuffer.pingPongIndex]);
    return (USBD_OK);
}

/**
 * @brief USB CDC高速接口反初始化
 * @return USBD_OK 反初始化成功
 */
static int8_t CDC_DeInit_HS()
{
    return (USBD_OK);
}


/**
 * @brief USB CDC控制命令处理函数
 * @param cmd 控制命令类型
 * @param pbuf 数据缓冲区指针
 * @param length 数据长度
 * @return USBD_OK 处理成功
 * @note 处理各种CDC控制命令，如线路编码设置、通信特性等
 */
static int8_t CDC_Control_HS(uint8_t cmd, uint8_t* pbuf, uint16_t length)
{
    switch (cmd)
    {
        case CDC_SEND_ENCAPSULATED_COMMAND:     // 发送封装命令
            break;

        case CDC_GET_ENCAPSULATED_RESPONSE:     // 获取封装响应
            break;

        case CDC_SET_COMM_FEATURE:              // 设置通信特性
            break;

        case CDC_GET_COMM_FEATURE:              // 获取通信特性
            break;

        case CDC_CLEAR_COMM_FEATURE:            // 清除通信特性
            break;

            /*******************************************************************************/
            /* 线路编码结构                                                                  */
            /*-----------------------------------------------------------------------------*/
            /* 偏移 | 字段        | 大小 | 值     | 描述                                    */
            /* 0    | dwDTERate   |   4  | 数字   | 数据终端速率，单位：比特/秒              */
            /* 4    | bCharFormat |   1  | 数字   | 停止位                                  */
            /*                                      0 - 1个停止位                         */
            /*                                      1 - 1.5个停止位                       */
            /*                                      2 - 2个停止位                         */
            /* 5    | bParityType |  1   | 数字   | 奇偶校验                                */
            /*                                      0 - 无校验                           */
            /*                                      1 - 奇校验                           */
            /*                                      2 - 偶校验                           */
            /*                                      3 - 标记                             */
            /*                                      4 - 空格                             */
            /* 6    | bDataBits   |   1   | 数字   | 数据位数 (5, 6, 7, 8 或 16)           */
            /*******************************************************************************/
        case CDC_SET_LINE_CODING:               // 设置线路编码
            break;

        case CDC_GET_LINE_CODING:               // 获取线路编码
            break;

        case CDC_SET_CONTROL_LINE_STATE:        // 设置控制线路状态
            break;

        case CDC_SEND_BREAK:                    // 发送中断信号
            break;

        default:
            break;
    }

    return (USBD_OK);
}


/**
 * @brief USB CDC数据接收回调函数
 * @param Buf 接收数据缓冲区指针
 * @param Len 接收数据长度指针
 * @return USBD_LL_PrepareReceive的返回值
 * @note 处理接收到的USB数据，实现ping-pong缓冲机制和LCD数据处理
 *       224字节包：触发ping-pong缓冲区切换（用于LCD数据传输）
 *       512字节包：LCD图像数据块，更新偏移量
 */
static int8_t CDC_Receive_HS(uint8_t* Buf, uint32_t* Len)
{
    // 注释掉的旧版本代码，现在使用LL版本
    // USBD_CDC_SetRxBuffer(&hUsbDeviceHS, frameBuffer[1]);
    // USBD_CDC_ReceivePacket(&hUsbDeviceHS);
    // Use LL version code instead

    // 保存接收到的数据长度
    electron.usbBuffer.receivedPacketLen = *Len;
    
    // 如果接收到224字节的包，切换ping-pong缓冲区
    // 这通常表示一帧LCD数据传输的开始
    if (electron.usbBuffer.receivedPacketLen == 224)
        electron.SwitchPingPongBuffer();

    // 准备下一次接收
    USBD_CDC_HandleTypeDef* hcdc = (USBD_CDC_HandleTypeDef*) (hUsbDeviceHS.pClassData);

    // 设置接收缓冲区为当前ping-pong缓冲区
    hcdc->RxBuffer = electron.GetPingPongBufferPtr();
    
    // 如果接收到512字节的包（LCD图像数据块），更新偏移量
    if (electron.usbBuffer.receivedPacketLen == 512)
    {
        electron.usbBuffer.rxDataOffset += 512;
    }

    // 准备接收下一个数据包
    return USBD_LL_PrepareReceive(&hUsbDeviceHS, CDC_OUT_EP, hcdc->RxBuffer, CDC_DATA_HS_OUT_PACKET_SIZE);
}


/**
 * @brief USB CDC数据发送函数
 * @param Buf 发送数据缓冲区指针
 * @param Len 发送数据长度
 * @return USBD_OK=成功，USBD_BUSY=忙碌
 * @note 通过USB CDC接口发送数据到主机，检查发送状态避免冲突
 */
uint8_t CDC_Transmit_HS(uint8_t* Buf, uint16_t Len)
{
    uint8_t result = USBD_OK;
    USBD_CDC_HandleTypeDef* hcdc = (USBD_CDC_HandleTypeDef*) hUsbDeviceHS.pClassData;
    
    // 检查发送状态，如果正在发送则返回忙碌
    if (hcdc->TxState != 0)
    {
        return USBD_BUSY;
    }
    
    // 设置发送缓冲区和数据长度
    USBD_CDC_SetTxBuffer(&hUsbDeviceHS, Buf, Len);
    // 启动数据包发送
    result = USBD_CDC_TransmitPacket(&hUsbDeviceHS);
    return result;
}

/**
 * @brief USB CDC发送完成回调函数
 * @param Buf 发送缓冲区指针（未使用）
 * @param Len 发送长度指针（未使用）
 * @param epnum 端点号（未使用）
 * @return USBD_OK 处理成功
 * @note 发送完成后的回调处理，当前实现为空
 */
static int8_t CDC_TransmitCplt_HS(uint8_t* Buf, uint32_t* Len, uint8_t epnum)
{
    uint8_t result = USBD_OK;
    UNUSED(Buf);        // 标记参数未使用
    UNUSED(Len);
    UNUSED(epnum);
    return result;
}
