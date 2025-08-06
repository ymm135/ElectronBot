#ifndef ELECTRONBOTSDK_ELECTRONLOWLEVEL_H
#define ELECTRONBOTSDK_ELECTRONLOWLEVEL_H

#include <iostream>
#include <cstdio>
#include <thread>
#include <opencv2/opencv.hpp>

/**
 * @brief ElectronBot底层SDK类
 * @details 提供与ElectronBot硬件的底层通信接口，包括：
 *          - USB通信管理
 *          - 图像数据传输（240x240 RGB）
 *          - 关节角度控制与反馈
 *          - 乒乓缓冲机制实现
 *          - 多线程同步处理
 */
class __declspec(dllexport) ElectronLowLevel
{
public:
    /**
     * @brief 默认构造函数
     * @details 使用默认的USB VID/PID (0x1001/0x8023)
     */
    ElectronLowLevel()
    = default;

    /**
     * @brief 带参数构造函数
     * @param _vid USB厂商ID
     * @param _pid USB产品ID
     * @details 允许自定义USB设备标识符
     */
    ElectronLowLevel(int _vid, int _pid) :
        USB_PID(_pid), USB_VID(_vid)
    {}

    /**
     * @brief 连接ElectronBot设备
     * @return true-连接成功，false-连接失败
     * @details 建立USB通信连接并启动同步线程
     */
    bool Connect();
    
    /**
     * @brief 断开ElectronBot设备连接
     * @return true-断开成功，false-断开失败
     * @details 停止同步线程并释放USB资源
     */
    bool Disconnect();
    
    /**
     * @brief 执行一次数据同步
     * @return true-同步成功，false-同步失败
     * @details 发送图像和控制数据，接收反馈数据
     */
    bool Sync();
    
    /**
     * @brief 设置图像源（OpenCV Mat格式）
     * @param _mat 输入图像矩阵，将自动缩放到240x240
     * @details 图像会被转换为RGB格式并存储到发送缓冲区
     */
    void SetImageSrc(const cv::Mat &_mat);
    
    /**
     * @brief 设置图像源（文件路径）
     * @param _filePath 图像文件路径
     * @details 从文件加载图像，自动缩放到240x240并转换为RGB
     */
    void SetImageSrc(const std::string &_filePath);
    
    /**
     * @brief 设置额外数据
     * @param _data 数据指针
     * @param _len 数据长度，默认32字节
     * @details 用于发送自定义控制数据到硬件
     */
    void SetExtraData(uint8_t* _data, uint32_t _len = 32);
    
    /**
     * @brief 设置关节角度
     * @param _j1-_j6 六个关节的目标角度（弧度）
     * @param _enable 是否使能所有关节
     * @details 将角度数据打包到额外数据缓冲区中发送
     */
    void SetJointAngles(float _j1, float _j2, float _j3, float _j4, float _j5, float _j6,
                        bool _enable = false);
    
    /**
     * @brief 获取当前关节角度
     * @param _jointAngles 输出数组，存储6个关节的当前角度
     * @details 从接收缓冲区解析硬件反馈的关节角度
     */
    void GetJointAngles(float* _jointAngles);
    
    /**
     * @brief 获取额外数据
     * @param _data 输出缓冲区，如果为nullptr则返回内部缓冲区指针
     * @return 数据指针
     * @details 获取从硬件接收到的额外数据
     */
    uint8_t* GetExtraData(uint8_t* _data = nullptr);

    // 公共成员变量
    int USB_VID = 0x1001;           ///< USB厂商ID，默认0x1001
    int USB_PID = 0x8023;           ///< USB产品ID，默认0x8023
    bool isConnected = false;       ///< 连接状态标志
    uint32_t timeStamp = 0;         ///< 时间戳，用于同步计时

private:
    // 乒乓缓冲相关
    uint8_t pingPongWriteIndex = 0;                 ///< 乒乓缓冲写入索引（0或1）
    
    // USB通信缓冲区
    uint8_t usbBuffer200[200]{};                    ///< USB通用缓冲区
    uint8_t frameBufferTx[2][240 * 240 * 3]{};     ///< 图像发送双缓冲区（172800字节×2）
    uint8_t extraDataBufferTx[2][32]{};            ///< 额外数据发送双缓冲区（32字节×2）
    uint8_t extraDataBufferRx[32]{};               ///< 额外数据接收缓冲区（32字节）
    
    // 多线程处理
    std::thread syncTaskHandle;                    ///< 同步任务线程句柄

    /**
     * @brief 接收USB数据包
     * @param _buffer 接收缓冲区
     * @param _packetCount 数据包数量
     * @param _packetSize 单个数据包大小
     * @return true-接收成功，false-接收失败
     * @details 使用USB Bulk传输接收指定数量和大小的数据包
     */
    static bool ReceivePacket(uint8_t* _buffer, uint32_t _packetCount, uint32_t _packetSize);
    
    /**
     * @brief 发送USB数据包
     * @param _buffer 发送缓冲区
     * @param _packetCount 数据包数量
     * @param _packetSize 单个数据包大小
     * @return true-发送成功，false-发送失败
     * @details 使用USB Bulk传输发送指定数量和大小的数据包
     */
    static bool TransmitPacket(uint8_t* _buffer, uint32_t _packetCount, uint32_t _packetSize);
    
    /**
     * @brief 同步任务线程函数
     * @param _obj ElectronLowLevel对象指针
     * @details 在独立线程中循环执行数据同步，实现实时通信
     */
    static void SyncTask(ElectronLowLevel* _obj);
};


#endif