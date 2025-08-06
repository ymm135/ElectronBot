/**
 * @file USBInterface.h
 * @brief USB通信接口定义
 * @details 定义了与ElectronBot硬件进行USB通信的底层接口函数
 *          支持Bulk传输、中断传输和控制传输
 */

#ifndef __USBTRANSMIT_H__
#define __USBTRANSMIT_H__

#include <Windows.h>
#include <sdkddkver.h>
#include <iostream>

using namespace std;

// DLL导出宏定义
#define DLL_API extern "C" _declspec(dllexport)

// USB端点定义
#define EP0     0x00    // 控制端点
#define EP1_IN  0x81    // 输入端点1（设备到主机）
#define EP1_OUT 0x01    // 输出端点1（主机到设备）

/**
 * @brief 扫描指定VID/PID的USB设备
 * @param _usbPid USB产品ID
 * @param _usbVid USB厂商ID
 * @return 找到的设备数量
 */
DLL_API
int USB_ScanDevice(int _usbPid, int _usbVid);

/**
 * @brief 打开指定索引的USB设备
 * @param _devIndex 设备索引
 * @return 成功返回true，失败返回false
 */
DLL_API
bool USB_OpenDevice(int _devIndex);

/**
 * @brief 关闭指定索引的USB设备
 * @param _devIndex 设备索引
 * @return 成功返回true，失败返回false
 */
DLL_API
bool USB_CloseDevice(int _devIndex);

/**
 * @brief 检查指定索引的USB设备是否可用
 * @param _devIndex 设备索引
 * @return 设备可用返回true，不可用返回false
 */
DLL_API
bool USB_CheckDevice(int _devIndex);

/**
 * @brief USB Bulk批量传输（发送数据）
 * @param _devIndex 设备索引
 * @param _pipeNum 管道编号（端点）
 * @param _sendBuffer 发送数据缓冲区
 * @param _len 数据长度
 * @param _timeout 超时时间（毫秒）
 * @return 成功返回true，失败返回false
 */
DLL_API
bool USB_BulkTransmit(int _devIndex, int _pipeNum, char* _sendBuffer, int _len, int _timeout);

/**
 * @brief USB Bulk批量传输（接收数据）
 * @param _device 设备索引
 * @param _pipeNum 管道编号（端点）
 * @param _data 接收数据缓冲区
 * @param _len 期望接收的数据长度
 * @param _timeout 超时时间（毫秒）
 * @return 实际接收到的数据长度，失败返回负值
 */
DLL_API
int USB_BulkReceive(int _device, int _pipeNum, char* _data, int _len, int _timeout);

/**
 * @brief USB中断传输（发送数据）
 * @param _devIndex 设备索引
 * @param _pipeNum 管道编号（端点）
 * @param _sendBuffer 发送数据缓冲区
 * @param _len 数据长度
 * @param _timeout 超时时间（毫秒）
 * @return 成功返回true，失败返回false
 */
DLL_API
bool USB_IntTransmit(int _devIndex, int _pipeNum, char* _sendBuffer, int _len, int _timeout);

/**
 * @brief USB中断传输（接收数据）
 * @param _device 设备索引
 * @param _pipeNum 管道编号（端点）
 * @param _data 接收数据缓冲区
 * @param _len 期望接收的数据长度
 * @param _timeout 超时时间（毫秒）
 * @return 实际接收到的数据长度，失败返回负值
 */
DLL_API
int USB_IntReceive(int _device, int _pipeNum, char* _data, int _len, int _timeout);

/**
 * @brief USB控制传输
 * @param _devIndex 设备索引
 * @param _requestType 请求类型
 * @param _request 请求代码
 * @param _value 值字段
 * @param _index 索引字段
 * @param _bytes 数据缓冲区
 * @param _size 数据大小
 * @param _timeout 超时时间（毫秒）
 * @return 成功返回true，失败返回false
 */
DLL_API
bool USB_CtrlData(int _devIndex, int _requestType, int _request, int _value,
                  int _index, char* _bytes, int _size, int _timeout);

#endif