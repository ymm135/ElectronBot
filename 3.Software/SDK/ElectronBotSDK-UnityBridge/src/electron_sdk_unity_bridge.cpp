/**
 * @file electron_sdk_unity_bridge.cpp
 * @brief ElectronBot Unity桥接实现
 * @details 实现Unity与ElectronBot硬件之间的通信桥梁
 */

#include "electron_sdk_unity_bridge.h"

using namespace cv;

/**
 * @enum EmojiSrcType_t
 * @brief 表情源类型枚举
 */
enum EmojiSrcType_t
{
    TYPE_NONE,     ///< 无表情源
    TYPE_PICTURE,  ///< 图片类型
    TYPE_VIDEO     ///< 视频类型
};

// 全局变量
EmojiSrcType_t emojiSrcType;        ///< 当前表情源类型
ElectronLowLevel robot;             ///< ElectronBot底层控制对象
Mat imgCamera;                      ///< 摄像头图像缓冲区
Mat imgEmoji;                       ///< 表情图像缓冲区
VideoCapture videoEmoji;            ///< 表情视频捕获对象
VideoCapture videoCamera(0);        ///< 摄像头捕获对象（默认设备0）

thread onUpdateTaskHandle;          ///< 更新任务线程句柄
bool isBusy = false;                ///< 忙碌状态标志

uint8_t robotExtraData[32];         ///< 机器人额外数据缓冲区
float robotJoints[6];               ///< 机器人关节角度缓冲区

/**
 * @brief 更新任务函数
 * @param _imgDataEmoji Unity传入的表情图像数据指针
 * @param _imgDataCamera Unity传入的摄像头图像数据指针
 * @param _width 图像宽度
 * @param _height 图像高度
 * @param _setJoints 关节角度数组
 * @param _enable 关节使能标志
 * @details 在独立线程中处理图像和关节数据，避免阻塞Unity主线程
 */
void OnUpdateTask(unsigned char* _imgDataEmoji, unsigned char* _imgDataCamera,
                  int _width, int _height, float* _setJoints, bool _enable)
{
#if 1  // 硬件连接模式
    if (robot.isConnected)
    {
        // 设置关节角度到硬件
        robot.SetJointAngles(_setJoints[0], _setJoints[1], _setJoints[2],
                             _setJoints[3], _setJoints[4], _setJoints[5], _enable);

        // 根据表情源类型处理图像
        switch (emojiSrcType)
        {
            case TYPE_PICTURE:  // 图片类型处理
            {
                emojiSrcType = TYPE_NONE;  // 图片只显示一次

                if (!imgEmoji.empty())
                {
                    // 发送图像到硬件屏幕
                    robot.SetImageSrc(imgEmoji);

                    // 图像处理：翻转、缩放、颜色转换
                    flip(imgEmoji, imgEmoji, -1);  // 水平和垂直翻转
                    resize(imgEmoji, imgEmoji, Size(_width, _height), cv::INTER_CUBIC);  // 缩放到Unity需要的尺寸
                    cvtColor(imgEmoji, imgEmoji, COLOR_RGB2BGRA);  // 转换为Unity的ARGB格式
                    memcpy(_imgDataEmoji, imgEmoji.data, imgEmoji.total() * imgEmoji.elemSize());  // 复制到Unity缓冲区
                }

                break;
            }
            case TYPE_VIDEO:
            {
                if (videoEmoji.get(CV_CAP_PROP_POS_FRAMES) < videoEmoji.get(CV_CAP_PROP_FRAME_COUNT))
                {
                    videoEmoji >> imgEmoji;
                    videoEmoji.set(CV_CAP_PROP_POS_FRAMES, videoEmoji.get(CV_CAP_PROP_POS_FRAMES) + 2);

                    if (!imgEmoji.empty())
                    {
                        robot.SetImageSrc(imgEmoji);

                        flip(imgEmoji, imgEmoji, -1);
                        //Resize Mat to match the array passed to it from C#
                        resize(imgEmoji, imgEmoji, Size(_width, _height), cv::INTER_CUBIC);
                        //Convert from RGB to ARGB
                        cvtColor(imgEmoji, imgEmoji, COLOR_RGB2BGRA);
                        memcpy(_imgDataEmoji, imgEmoji.data, imgEmoji.total() * imgEmoji.elemSize());
                    }
                }
                break;
            }
            case TYPE_NONE:
                break;
        }
        robot.Sync();
        robot.GetJointAngles(robotJoints);
    }

#else
    switch (emojiSrcType)
    {
        case TYPE_PICTURE:
        {
            emojiSrcType = TYPE_NONE;

            if (!imgEmoji.empty())
            {
                flip(imgEmoji, imgEmoji, -1);
                //Resize Mat to match the array passed to it from C#
                resize(imgEmoji, imgEmoji, Size(_width, _height), cv::INTER_CUBIC);
                //Convert from RGB to ARGB
                cvtColor(imgEmoji, imgEmoji, COLOR_RGB2BGRA);
                memcpy(_imgDataEmoji, imgEmoji.data, imgEmoji.total() * imgEmoji.elemSize());
            }

            break;
        }
        case TYPE_VIDEO:
        {
            if (videoEmoji.get(CV_CAP_PROP_POS_FRAMES) < videoEmoji.get(CV_CAP_PROP_FRAME_COUNT))
            {
                videoEmoji >> imgEmoji;

                videoEmoji.set(CV_CAP_PROP_POS_FRAMES, videoEmoji.get(CV_CAP_PROP_POS_FRAMES) + 2);

                if (!imgEmoji.empty())
                {
                    flip(imgEmoji, imgEmoji, -1);
                    //Resize Mat to match the array passed to it from C#
                    resize(imgEmoji, imgEmoji, Size(_width, _height), cv::INTER_CUBIC);
                    //Convert from RGB to ARGB
                    cvtColor(imgEmoji, imgEmoji, COLOR_RGB2BGRA);
                    memcpy(_imgDataEmoji, imgEmoji.data, imgEmoji.total() * imgEmoji.elemSize());
                }
            } else
            {
                emojiSrcType = TYPE_NONE;
            }

            break;
        }
    }
#endif

    videoCamera >> imgCamera;
    if (!imgCamera.empty())
    {
        //flip horizontally
        flip(imgCamera, imgCamera, -1);
        //Resize Mat to match the array passed to it from C#
        resize(imgCamera, imgCamera, Size(_width, _height), cv::INTER_CUBIC);
        //Convert from RGB to ARGB
        cvtColor(imgCamera, imgCamera, COLOR_RGB2BGRA);
        memcpy(_imgDataCamera, imgCamera.data, imgCamera.total() * imgCamera.elemSize());
    }

    isBusy = false;
}


/**
 * @brief 关键帧变化回调实现
 * @param _filePath 媒体文件路径
 * @details 根据文件扩展名识别媒体类型并加载：
 *          - .mp4：视频文件，创建VideoCapture对象
 *          - .jpg/.png/.bmp：图片文件，使用imread加载
 *          - 其他：设置为无表情源
 */
void Native_OnKeyFrameChange(const char* _filePath)
{
    string s(_filePath);
    if (s.find(".mp4") != string::npos)
    {
        videoEmoji = VideoCapture(_filePath);  // 打开视频文件
        emojiSrcType = TYPE_VIDEO;
    } else if (s.find(".jpg") != string::npos ||
               s.find(".png") != string::npos ||
               s.find(".bmp") != string::npos)
    {
        imgEmoji = imread(_filePath);  // 加载图片文件
        emojiSrcType = TYPE_PICTURE;
    } else
    {
        emojiSrcType = TYPE_NONE;  // 不支持的文件类型
    }
}

/**
 * @brief SDK初始化实现
 * @details Unity启动时调用，建立与ElectronBot的连接
 */
void Native_OnInit()
{
    robot.Connect();
}

/**
 * @brief 固定更新回调实现
 * @param _imgDataEmoji Unity表情图像数据缓冲区
 * @param _imgDataCamera Unity摄像头图像数据缓冲区
 * @param _width 图像宽度
 * @param _height 图像高度
 * @param _setJoints 关节角度数组
 * @param _enable 关节使能标志
 * @return 当前机器人关节角度数组
 * @details 异步处理图像和关节数据，避免阻塞Unity渲染线程
 */
float* Native_OnFixUpdate(unsigned char* _imgDataEmoji, unsigned char* _imgDataCamera,
                          int _width, int _height, float* _setJoints, bool _enable)
{
    // 避免重复启动更新任务
    if (!isBusy)
    {
        isBusy = true;

        // 在独立线程中执行更新任务
        onUpdateTaskHandle = std::thread(OnUpdateTask, _imgDataEmoji, _imgDataCamera,
                                         _width, _height, _setJoints, _enable);
        onUpdateTaskHandle.detach();  // 分离线程，自动清理
    }

    return robotJoints;  // 返回当前关节角度
}

/**
 * @brief SDK退出清理实现
 * @details Unity退出时调用，释放所有资源并断开连接
 */
void Native_OnExit()
{
    // 释放摄像头资源
    if (videoCamera.isOpened())
        videoCamera.release();

    // 释放表情视频资源
    if (videoEmoji.isOpened())
        videoEmoji.release();

    // 释放图像缓冲区
    imgCamera.release();
    imgEmoji.release();

    // 断开机器人连接
    robot.Disconnect();
}

