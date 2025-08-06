#include "electron_low_level.h"
#include "USBInterface.h"

/**
 * @brief 执行一次数据同步操作
 * @return true-同步成功，false-设备未连接
 * @details 启动新的同步线程，发送图像和控制数据到硬件，
 *          并接收反馈数据。每次调用会增加时间戳计数器。
 */
bool ElectronLowLevel::Sync()
{
    if (isConnected)
    {
        // 等待上一个同步任务完成
        if (syncTaskHandle.joinable())
            syncTaskHandle.join();

        // 启动新的同步任务线程
        syncTaskHandle = std::thread(SyncTask, this);

        timeStamp++;  // 增加时间戳
        return true;
    }

    return false;
}

/**
 * @brief 设置图像源（OpenCV Mat格式）
 * @param _mat 输入图像矩阵
 * @details 将输入图像缩放到240x240像素，转换为RGB格式，
 *          并复制到当前乒乓缓冲区中。图像数据大小为172800字节。
 */
void ElectronLowLevel::SetImageSrc(const cv::Mat &_mat)
{
    cv::Mat temp;
    resize(_mat, temp, cv::Size(240, 240));           // 缩放到240x240
    cvtColor(temp, temp, CV_BGRA2RGB);                // 转换为RGB格式
    std::memcpy(frameBufferTx[pingPongWriteIndex], temp.data, 240 * 240 * 3);  // 复制到发送缓冲区
}

/**
 * @brief 设置图像源（文件路径）
 * @param _filePath 图像文件路径
 * @details 从文件加载图像，自动缩放到240x240并转换为RGB格式，
 *          然后存储到当前乒乓缓冲区中。
 */
void ElectronLowLevel::SetImageSrc(const string &_filePath)
{
    cv::Mat temp = cv::imread(_filePath);             // 从文件加载图像
    resize(temp, temp, cv::Size(240, 240));           // 缩放到240x240
    cvtColor(temp, temp, CV_BGRA2RGB);                // 转换为RGB格式
    std::memcpy(frameBufferTx[pingPongWriteIndex], temp.data, 240 * 240 * 3);  // 复制到发送缓冲区
}

/**
 * @brief 设置额外数据
 * @param _data 数据指针
 * @param _len 数据长度，最大32字节
 * @details 将自定义数据复制到额外数据发送缓冲区，
 *          用于发送关节角度等控制信息。
 */
void ElectronLowLevel::SetExtraData(uint8_t* _data, uint32_t _len)
{
    if (_len <= 32)  // 限制最大长度为32字节
        memcpy(extraDataBufferTx[pingPongWriteIndex], _data, _len);
}

/**
 * @brief 获取接收到的额外数据
 * @param _data 输出缓冲区，可选参数
 * @return 额外数据缓冲区指针
 * @details 返回从硬件接收到的32字节额外数据，
 *          通常包含当前关节角度等反馈信息。
 */
uint8_t* ElectronLowLevel::GetExtraData(uint8_t* _data)
{
    if (_data != nullptr)
        memcpy(_data, extraDataBufferRx, 32);  // 复制到用户缓冲区

    return extraDataBufferRx;  // 返回内部缓冲区指针
}


/**
 * @brief 接收USB数据包
 * @param _buffer 接收缓冲区指针
 * @param _packetCount 要接收的数据包数量
 * @param _packetSize 单个数据包大小（字节）
 * @return true-接收成功，false-接收失败
 * @details 使用USB Bulk传输从EP1_IN端点接收指定数量的数据包。
 *          每个数据包都会重试直到接收完整，超时时间为100ms。
 */
bool ElectronLowLevel::ReceivePacket(uint8_t* _buffer, uint32_t _packetCount, uint32_t _packetSize)
{
    uint32_t packetCount = _packetCount;
    uint32_t ret;
    do
    {
        do
        {
            // 调用USB接口接收数据，重试直到接收到完整数据包
            ret = USB_BulkReceive(0, EP1_IN, reinterpret_cast<char*>(_buffer), _packetSize, 100);
        } while (ret != _packetSize);  // 确保接收到完整的数据包

        packetCount--;  // 减少待接收包计数
    } while (packetCount > 0);  // 继续接收剩余数据包

    return packetCount == 0;  // 返回是否成功接收所有数据包
}

/**
 * @brief 发送USB数据包
 * @param _buffer 发送缓冲区指针
 * @param _packetCount 要发送的数据包数量
 * @param _packetSize 单个数据包大小（字节）
 * @return true-发送成功，false-发送失败
 * @details 使用USB Bulk传输向EP1_OUT端点发送指定数量的数据包。
 *          每个数据包都会重试直到发送成功，超时时间为100ms。
 */
bool ElectronLowLevel::TransmitPacket(uint8_t* _buffer, uint32_t _packetCount, uint32_t _packetSize)
{
    uint32_t packetCount = _packetCount;
    uint32_t dataOffset = 0;  // 数据偏移量
    uint32_t ret;
    do
    {
        do
        {
            // 调用USB接口发送数据，重试直到发送成功
            ret = USB_BulkTransmit(0, EP1_OUT,
                                   reinterpret_cast<char*>(_buffer) + dataOffset,
                                   _packetSize, 100);
        } while (!ret);  // 重试直到发送成功

        dataOffset += _packetSize;  // 移动到下一个数据包的位置
        packetCount--;  // 减少待发送包计数
    } while (packetCount > 0);  // 继续发送剩余数据包

    return packetCount == 0;
}


/**
 * @brief 连接ElectronBot设备
 * @return true-连接成功，false-连接失败
 * @details 扫描指定VID/PID的USB设备，如果找到则打开设备连接。
 *          成功连接后设置连接状态标志并重置时间戳。
 */
bool ElectronLowLevel::Connect()
{
    // 扫描指定VID/PID的USB设备
    int devNum = USB_ScanDevice(USB_PID, USB_VID);

    if (devNum > 0)  // 找到设备
    {
        if (USB_OpenDevice(0))  // 打开第一个设备
        {
            isConnected = true;  // 设置连接状态
            timeStamp = 0;       // 重置时间戳

            return true;
        }
    }

    return false;
}

/**
 * @brief 断开ElectronBot设备连接
 * @return true-断开成功，false-断开失败
 * @details 等待同步线程结束，然后关闭USB设备连接。
 *          成功断开后清除连接状态标志。
 */
bool ElectronLowLevel::Disconnect()
{
    // 等待同步线程结束
    if (syncTaskHandle.joinable())
        syncTaskHandle.join();

    // 关闭USB设备连接
    if (isConnected && USB_CloseDevice(0))
    {
        isConnected = false;  // 清除连接状态
        return true;
    }

    return false;
}


/**
 * @brief 同步任务线程函数
 * @param _obj ElectronLowLevel对象指针
 * @details 在独立线程中执行数据同步，实现乒乓缓冲机制：
 *          1. 切换乒乓缓冲区索引
 *          2. 循环4次发送图像数据（每次84个512字节包）
 *          3. 每次循环先接收32字节反馈数据
 *          4. 最后发送图像尾部数据和控制数据（224字节包）
 *          
 *          数据传输格式：
 *          - 图像数据：240×240×3=172800字节，分4次传输
 *          - 每次：84×512=43008字节图像数据 + 192字节尾部 + 32字节控制
 *          - 总计：4×43008 + 4×192 + 4×32 = 172800 + 896 = 173696字节
 */
void ElectronLowLevel::SyncTask(ElectronLowLevel* _obj)
{
    uint32_t frameBufferOffset = 0;  // 帧缓冲区偏移量
    uint8_t index = _obj->pingPongWriteIndex;  // 保存当前缓冲区索引
    
    // 切换乒乓缓冲区索引，实现双缓冲机制
    _obj->pingPongWriteIndex = _obj->pingPongWriteIndex == 0 ? 1 : 0;
    
    // 分4次传输完整的图像数据
    for (int p = 0; p < 4; p++)
    {
        // 等待MCU请求并接收32字节额外数据（关节反馈等）
        _obj->ReceivePacket(reinterpret_cast<uint8_t*>(_obj->extraDataBufferRx),
                            1, 32);

        // 发送84个512字节的图像数据包（43008字节）
        _obj->TransmitPacket(reinterpret_cast<uint8_t*>(_obj->frameBufferTx[index]) + frameBufferOffset,
                             84, 512);
        frameBufferOffset += 43008;  // 移动到下一段数据

        // 准备帧尾数据：192字节图像尾部 + 32字节控制数据
        memcpy(_obj->usbBuffer200, reinterpret_cast<uint8_t*>(_obj->frameBufferTx[index]) + frameBufferOffset,
               192);  // 复制192字节图像尾部数据
        memcpy(_obj->usbBuffer200 + 192, reinterpret_cast<uint8_t*>(_obj->extraDataBufferTx[index]), 32);  // 复制32字节控制数据

        // 发送帧尾和控制数据（224字节包）
        _obj->TransmitPacket(_obj->usbBuffer200, 1, 224);
        frameBufferOffset += 192;  // 移动到下一段图像尾部
    }
}

/**
 * @brief 设置关节角度
 * @param _j1-_j6 六个关节的目标角度（弧度）
 * @param _enable 是否使能所有关节
 * @details 将6个关节角度打包到32字节的额外数据缓冲区中：
 *          - 字节0：使能标志（1=使能，0=禁用）
 *          - 字节1-4：关节1角度（float，小端序）
 *          - 字节5-8：关节2角度（float，小端序）
 *          - ...依此类推
 *          - 字节25-28：关节6角度（float，小端序）
 *          - 字节29-31：保留
 */
void ElectronLowLevel::SetJointAngles(float _j1, float _j2, float _j3, float _j4, float _j5, float _j6,
                                      bool _enable)
{
    // 将关节角度存储到临时数组
    float jointAngleSetPoints[6];
    jointAngleSetPoints[0] = _j1;
    jointAngleSetPoints[1] = _j2;
    jointAngleSetPoints[2] = _j3;
    jointAngleSetPoints[3] = _j4;
    jointAngleSetPoints[4] = _j5;
    jointAngleSetPoints[5] = _j6;

    // 设置使能标志（第0字节）
    extraDataBufferTx[pingPongWriteIndex][0] = _enable ? 1 : 0;
    
    // 将6个float角度值按字节序列化到缓冲区
    for (int j = 0; j < 6; j++)  // 遍历6个关节
        for (int i = 0; i < 4; i++)  // 每个float占4字节
        {
            auto* b = (unsigned char*) &(jointAngleSetPoints[j]);  // 获取float的字节指针
            extraDataBufferTx[pingPongWriteIndex][j * 4 + i + 1] = *(b + i);  // 按小端序存储
        }
}

/**
 * @brief 获取当前关节角度
 * @param _jointAngles 输出数组，存储6个关节的当前角度
 * @details 从接收缓冲区解析硬件反馈的关节角度：
 *          - 字节0：状态标志（通常忽略）
 *          - 字节1-4：关节1当前角度（float，小端序）
 *          - 字节5-8：关节2当前角度（float，小端序）
 *          - ...依此类推
 *          - 字节25-28：关节6当前角度（float，小端序）
 */
void ElectronLowLevel::GetJointAngles(float* _jointAngles)
{
    // 从接收缓冲区解析6个关节的当前角度
    for (int j = 0; j < 6; j++)
    {
        // 直接将4字节数据转换为float（小端序）
        _jointAngles[j] = *((float*) (extraDataBufferRx + 4 * j + 1));
    }
}

