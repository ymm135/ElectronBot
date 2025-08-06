/**
 * @file electron_player.h
 * @brief ElectronBot播放器类定义
 * @details 提供高级的机器人控制接口，支持动作文件播放、姿态控制等功能
 *          封装了底层USB通信，提供更友好的用户接口
 */

#ifndef ELECTRONBOTSDK_ELECTRONPLAYER_H
#define ELECTRONBOTSDK_ELECTRONPLAYER_H

#include <iostream>
#include <cstdio>
#include <thread>
#include "electron_low_level.h"

/**
 * @class ElectronPlayer
 * @brief ElectronBot高级控制类
 * @details 提供机器人的高级控制功能：
 *          - 动作文件播放
 *          - 实时姿态控制
 *          - 播放速度调节
 *          - 连接管理
 */
class __declspec(dllexport) ElectronPlayer
{
public:
    /**
     * @brief 默认构造函数
     * @details 使用默认的VID/PID创建ElectronPlayer实例
     */
    ElectronPlayer()
    {
        lowLevelHandle = new ElectronLowLevel();
    }

    /**
     * @brief 带参数构造函数
     * @param _vid USB厂商ID
     * @param _pid USB产品ID
     * @details 使用指定的VID/PID创建ElectronPlayer实例
     */
    ElectronPlayer(int _vid, int _pid) :
        USB_PID(_pid), USB_VID(_vid)
    {
        lowLevelHandle = new ElectronLowLevel(_vid, _pid);
    }

    /**
     * @brief 析构函数
     * @details 释放底层句柄资源
     */
    ~ElectronPlayer()
    {
        delete (lowLevelHandle);
    }

    /**
     * @struct RobotPose_t
     * @brief 机器人姿态结构体
     * @details 定义6个关节的角度值（弧度）
     */
    struct RobotPose_t
    {
        float j1;  ///< 关节1角度
        float j2;  ///< 关节2角度
        float j3;  ///< 关节3角度
        float j4;  ///< 关节4角度
        float j5;  ///< 关节5角度
        float j6;  ///< 关节6角度
    };
    
    RobotPose_t currentPose{0, 0, 0, 0, 0, 0};  ///< 当前机器人姿态
    ElectronLowLevel* lowLevelHandle;  ///< 底层通信句柄

    /**
     * @brief 连接到ElectronBot设备
     * @return 连接成功返回true，失败返回false
     */
    bool Connect();
    
    /**
     * @brief 断开与ElectronBot设备的连接
     * @return 断开成功返回true，失败返回false
     */
    bool Disconnect();
    
    /**
     * @brief 播放动作文件
     * @param _filePath 动作文件路径
     * @details 以默认速度播放指定的动作文件
     */
    void Play(const std::string &_filePath);
    
    /**
     * @brief 以指定速度播放动作文件
     * @param _filePath 动作文件路径
     * @param _speedRatio 播放速度比例（1.0为正常速度）
     */
    void Play(const std::string &_filePath, float _speedRatio);
    
    /**
     * @brief 停止当前播放
     */
    void Stop();
    
    /**
     * @brief 设置播放速度
     * @param _ratio 速度比例（1.0为正常速度，>1.0加速，<1.0减速）
     */
    void SetPlaySpeed(float _ratio);
    
    /**
     * @brief 设置机器人姿态
     * @param _pose 目标姿态
     */
    void SetPose(const RobotPose_t &_pose);
    
    /**
     * @brief 获取当前机器人姿态
     * @return 当前姿态
     */
    RobotPose_t GetPose();

    int USB_VID = 0x1001;      ///< USB厂商ID
    int USB_PID = 0x8023;      ///< USB产品ID
    bool isConnected = false;  ///< 连接状态标志


private:
    bool isPlaying = false;          ///< 播放状态标志
    float playSpeedRatio = 1.0f;     ///< 当前播放速度比例
    std::thread playTaskHandle;      ///< 播放任务线程句柄

    /**
     * @brief 播放任务线程函数
     * @param _obj ElectronPlayer对象指针
     * @param _filePath 动作文件路径
     * @param _speedRatio 播放速度比例
     * @details 在独立线程中执行动作文件播放，解析文件中的关节角度序列
     *          并按指定速度发送给机器人硬件
     */
    static void PlayTask(ElectronPlayer* _obj, const std::string &_filePath, float _speedRatio);
};


#endif