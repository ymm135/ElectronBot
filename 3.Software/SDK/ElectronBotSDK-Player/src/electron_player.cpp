/**
 * @file electron_player.cpp
 * @brief ElectronBot播放器类实现
 * @details 实现高级的机器人控制功能，包括连接管理、媒体播放等
 */

#include "electron_player.h"
#include <opencv2/opencv.hpp>

/**
 * @brief 连接到ElectronBot设备
 * @return 连接成功返回true，失败返回false
 * @details 调用底层接口建立USB连接
 */
bool ElectronPlayer::Connect()
{
    isConnected = lowLevelHandle->Connect();
    return isConnected;
}

/**
 * @brief 断开与ElectronBot设备的连接
 * @return 断开成功返回true，失败返回false
 * @details 等待播放线程结束后断开USB连接
 */
bool ElectronPlayer::Disconnect()
{
    // 等待播放任务线程结束
    if (playTaskHandle.joinable())
        playTaskHandle.join();
    return lowLevelHandle->Disconnect();
}


/**
 * @brief 播放媒体文件
 * @param _filePath 媒体文件路径
 * @details 根据文件扩展名自动识别文件类型：
 *          - 图片文件（jpg/png/bmp）：直接显示到屏幕
 *          - 视频文件（mp4/mov）：在独立线程中逐帧播放
 */
void ElectronPlayer::Play(const std::string &_filePath)
{
    // 图片文件类型处理
    if (_filePath.find(".jpg") != std::string::npos ||
        _filePath.find(".png") != std::string::npos ||
        _filePath.find(".bmp") != std::string::npos)
    {
        // 加载图片并设置到屏幕
        lowLevelHandle->SetImageSrc(cv::imread(_filePath));
        lowLevelHandle->Sync();  // 立即同步显示
    }
    // 视频文件类型处理
    else if (_filePath.find(".mp4") != std::string::npos ||
             _filePath.find(".mov") != std::string::npos)
    {
        if (isConnected)
        {
            isPlaying = true;
            // 在独立线程中播放视频
            playTaskHandle = std::thread(PlayTask, this, _filePath, playSpeedRatio);
        }
    }
}

/**
 * @brief 以指定速度播放媒体文件
 * @param _filePath 媒体文件路径
 * @param _speedRatio 播放速度比例
 */
void ElectronPlayer::Play(const std::string &_filePath, float _speedRatio)
{
    playSpeedRatio = _speedRatio;
    Play(_filePath);
}

/**
 * @brief 停止当前播放
 * @details 设置播放标志为false，播放线程会自动退出
 */
void ElectronPlayer::Stop()
{
    isPlaying = false;
}

/**
 * @brief 设置机器人姿态
 * @param _pose 目标姿态
 * @details 当前版本未实现，预留接口
 */
void ElectronPlayer::SetPose(const ElectronPlayer::RobotPose_t &_pose)
{
    // TODO: 实现姿态设置功能
}

/**
 * @brief 获取当前机器人姿态
 * @return 当前姿态
 * @details 当前版本返回默认姿态，预留接口
 */
ElectronPlayer::RobotPose_t ElectronPlayer::GetPose()
{
    // TODO: 从硬件获取实际姿态
    return ElectronPlayer::RobotPose_t();
}

/**
 * @brief 设置播放速度
 * @param _ratio 速度比例（必须大于0）
 * @details 只有当比例大于0时才会更新播放速度
 */
void ElectronPlayer::SetPlaySpeed(float _ratio)
{
    if (_ratio > 0)
    {
        playSpeedRatio = _ratio;
    }
}

/**
 * @brief 播放任务线程函数
 * @param _obj ElectronPlayer对象指针
 * @param _filePath 视频文件路径
 * @param _speedRatio 播放速度比例
 * @details 在独立线程中逐帧播放视频文件：
 *          1. 打开视频文件
 *          2. 根据速度比例跳帧播放
 *          3. 将每帧图像发送到ElectronBot屏幕
 *          4. 播放完成或停止时退出
 */
void ElectronPlayer::PlayTask(ElectronPlayer* _obj, const std::string &_filePath, float _speedRatio)
{
    cv::VideoCapture video(_filePath);  // 打开视频文件
    cv::Mat frame;  // 当前帧图像

    auto totalFrameCount = video.get(CV_CAP_PROP_FRAME_COUNT);  // 获取总帧数
    long index = 1;  // 当前帧索引

    // 循环播放直到停止或播放完成
    while (_obj->isPlaying && index < totalFrameCount)
    {
        video >> frame;  // 读取当前帧
        index += (long) _speedRatio;  // 根据速度比例跳帧
        video.set(CV_CAP_PROP_POS_FRAMES, index);  // 设置下一帧位置

        // 将帧图像发送到ElectronBot
        _obj->lowLevelHandle->SetImageSrc(frame);
        _obj->lowLevelHandle->Sync();
    }

    _obj->isPlaying = false;  // 播放结束，重置状态
}

