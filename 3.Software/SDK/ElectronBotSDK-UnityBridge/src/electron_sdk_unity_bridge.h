/**
 * @file electron_sdk_unity_bridge.h
 * @brief ElectronBot Unity桥接接口
 * @details 为Unity引擎提供C风格的API接口，用于在Unity中控制ElectronBot
 *          支持图像传输、关节控制、动作播放等功能
 */

#ifndef CPPTOUNITY_LIBRARY_H
#define CPPTOUNITY_LIBRARY_H

#include <Windows.h>
#include <sdkddkver.h>
#include <iostream>
#include "electron_low_level.h"

using namespace std;

// DLL导出宏定义，用于Unity P/Invoke调用
#define DLL_API extern "C" _declspec(dllexport)

/**
 * @brief 关键帧变化回调
 * @param _filePath 动作文件路径
 * @details Unity调用此函数通知SDK播放指定的动作文件
 */
DLL_API
void Native_OnKeyFrameChange(const char* _filePath);

/**
 * @brief 固定更新回调（Unity FixedUpdate）
 * @param _imgDataEmoji 表情图像数据指针
 * @param _imgDataCamera 摄像头图像数据指针
 * @param _width 图像宽度
 * @param _height 图像高度
 * @param _setJoints 关节角度数组指针（6个float值）
 * @param _enable 是否使能关节控制
 * @return 返回当前关节角度数组指针
 * @details Unity每个固定时间步调用此函数，传递图像和关节数据给ElectronBot
 */
DLL_API
float* Native_OnFixUpdate(unsigned char* _imgDataEmoji, unsigned char* _imgDataCamera,
                          int _width, int _height, float* _setJoints, bool _enable);

/**
 * @brief 初始化SDK
 * @details Unity启动时调用，初始化ElectronBot连接和相关资源
 */
DLL_API
void Native_OnInit();

/**
 * @brief 退出SDK
 * @details Unity退出时调用，清理资源并断开ElectronBot连接
 */
DLL_API
void Native_OnExit();

#endif
