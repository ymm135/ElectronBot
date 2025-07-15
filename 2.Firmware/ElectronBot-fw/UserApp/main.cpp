#include <cmath>
#include "common_inc.h"
#include "screen.h"
#include "robot.h"

// 全局变量定义
Robot electron(&hspi1, &hi2c1);  // ElectronBot机器人实例，使用SPI1和I2C1
float jointSetPoints[6];         // 6个关节的目标角度数组
bool isEnabled = false;          // 关节使能状态标志

/* Thread Definitions -----------------------------------------------------*/


/* Timer Callbacks -------------------------------------------------------*/


/* Default Entry -------------------------------------------------------*/
/**
 * @brief 主程序入口函数
 * @note 负责初始化系统、处理USB通信、更新LCD显示和控制关节运动
 */
void Main(void)
{
    // 系统启动延时，等待硬件稳定
    HAL_Delay(2000);
    
    // 初始化LCD显示屏
    electron.lcd->Init(Screen::DEGREE_0);        // 0度方向初始化
    electron.lcd->SetWindow(0, 239, 0, 239);     // 设置显示窗口为240x240全屏

    // 初始化所有关节到零位
    electron.UpdateJointAngle(electron.joint[1], 0);
    electron.UpdateJointAngle(electron.joint[2], 0);
    electron.UpdateJointAngle(electron.joint[3], 0);
    electron.UpdateJointAngle(electron.joint[4], 0);
    electron.UpdateJointAngle(electron.joint[5], 0);
    electron.UpdateJointAngle(electron.joint[6], 0);

    float t = 0;  // 时间变量（用于调试）

    // 主循环
    while (true)
    {
#if 1
        // LCD显示分4次传输，每次传输60行（240x60x3字节）
        for (int p = 0; p < 4; p++)
        {
            // 发送当前关节角度数据到上位机
            for (int j = 0; j < 6; j++)
                for (int i = 0; i < 4; i++)
                {
                    auto* b = (unsigned char*) &(electron.joint[j + 1].angle);
                    electron.usbBuffer.extraDataTx[j * 4 + i + 1] = *(b + i);  // 将float转换为字节数组
                }
            electron.SendUsbPacket(electron.usbBuffer.extraDataTx, 32);  // 发送32字节数据包

            // 等待接收上位机发送的数据包（最后一个包是224字节）
            electron.ReceiveUsbPacketUntilSizeIs(224);

            // 解析接收到的控制数据
            uint8_t* ptr = electron.GetExtraDataRxPtr();
            
            // 检查使能状态是否改变
            if (isEnabled != (bool) ptr[0])
            {
                isEnabled = ptr[0];
                // 同时设置所有关节的使能状态
                electron.SetJointEnable(electron.joint[1], isEnabled);
                electron.SetJointEnable(electron.joint[2], isEnabled);
                electron.SetJointEnable(electron.joint[3], isEnabled);
                electron.SetJointEnable(electron.joint[4], isEnabled);
                electron.SetJointEnable(electron.joint[5], isEnabled);
                electron.SetJointEnable(electron.joint[6], isEnabled);
            }
            
            // 解析6个关节的目标角度
            for (int j = 0; j < 6; j++)
            {
                jointSetPoints[j] = *((float*) (ptr + 4 * j + 1));
            }

            // 等待LCD传输完成
            while (electron.lcd->isBusy);
            
            // 分段传输LCD数据
            if (p == 0)
                electron.lcd->WriteFrameBuffer(electron.GetLcdBufferPtr(),
                                               60 * 240 * 3);        // 第一段，新建传输
            else
                electron.lcd->WriteFrameBuffer(electron.GetLcdBufferPtr(),
                                               60 * 240 * 3, true);  // 后续段，追加传输
        }
        HAL_Delay(1);  // 短暂延时
#endif


        t += 0.01;  // 时间递增（用于调试）

        // 更新所有关节到目标角度
        electron.UpdateJointAngle(electron.joint[1], jointSetPoints[0]);
        electron.UpdateJointAngle(electron.joint[2], jointSetPoints[1]);
        electron.UpdateJointAngle(electron.joint[3], jointSetPoints[2]);
        electron.UpdateJointAngle(electron.joint[4], jointSetPoints[3]);
        electron.UpdateJointAngle(electron.joint[5], jointSetPoints[4]);
        electron.UpdateJointAngle(electron.joint[6], jointSetPoints[5]);

        HAL_Delay(1);  // 控制循环延时

        // 调试用：可以设置单个关节做正弦波运动
        // electron.UpdateJointAngle(electron.joint[ANY], 65 + 75 * std::sin(t));

        // 串口输出当前关节目标角度（调试用）
        printf("%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
               jointSetPoints[0], jointSetPoints[1], jointSetPoints[2],
               jointSetPoints[3], jointSetPoints[4], jointSetPoints[5]);
    }
}

/**
 * @brief SPI传输完成回调函数
 * @param hspi SPI句柄指针
 * @note 当LCD的SPI传输完成时调用，清除忙碌标志
 */
extern "C"
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi)
{
    electron.lcd->isBusy = false;  // 清除LCD忙碌标志
}