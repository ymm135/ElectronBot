using UnityEngine;
using System;
using System.Runtime.InteropServices;

/// <summary>
/// Unity与C++原生库交互管理器
/// 负责处理图像数据传输、机器人关节数据同步和时间轴帧变化通知
/// </summary>
public class UnityGetImageFromCpp : MonoBehaviour
{
    /// <summary>表情显示平面对象</summary>
    public GameObject texturePlaneEmoji;
    /// <summary>摄像头显示平面对象</summary>
    public GameObject texturePlaneCamera;
    /// <summary>纹理宽度</summary>
    public int textureWidth = 512;
    /// <summary>纹理高度</summary>
    public int textureHeight = 512;

    /// <summary>表情纹理对象</summary>
    private Texture2D mTexEmoji;
    /// <summary>表情像素数据数组</summary>
    private Color32[] mPixel32Emoji;
    /// <summary>表情像素数据的GC句柄</summary>
    private GCHandle mPixelHandleEmoji;
    /// <summary>表情像素数据的指针</summary>
    private IntPtr mPixelPtrEmoji;
    /// <summary>摄像头纹理对象</summary>
    private Texture2D mTexCamera;
    /// <summary>摄像头像素数据数组</summary>
    private Color32[] mPixel32Camera;
    /// <summary>摄像头像素数据的GC句柄</summary>
    private GCHandle mPixelHandleCamera;
    /// <summary>摄像头像素数据的指针</summary>
    private IntPtr mPixelPtrCamera;

    /// <summary>机器人控制器引用</summary>
    public RobotController robot;
    /// <summary>姿态编辑器引用</summary>
    public PoseEditor poseEditor;


// #if UNITY_EDITOR
    /// <summary>通知原生库关键帧变化</summary>
    [DllImport("ElectronBotSDK-UnityBridge")]
    private static extern void Native_OnKeyFrameChange(string argString);

    /// <summary>固定更新调用，传递图像数据和关节数据</summary>
    [DllImport("ElectronBotSDK-UnityBridge")]
    private static extern IntPtr Native_OnFixUpdate(IntPtr dataEmoji, IntPtr dataCamera, int width, int height,
        IntPtr setJoints, bool enable);

    /// <summary>退出时清理原生库资源</summary>
    [DllImport("ElectronBotSDK-UnityBridge")]
    private static extern void Native_OnExit();

    /// <summary>初始化原生库</summary>
    [DllImport("ElectronBotSDK-UnityBridge")]
    private static extern void Native_OnInit();


    /// <summary>
    /// 初始化方法，设置原生库和纹理
    /// </summary>
    void Start()
    {
        // while (!RunExecutable.is_camera_opened) ;
        Native_OnInit();
        InitTexture();
        texturePlaneEmoji.GetComponent<Renderer>().material.mainTexture = mTexEmoji;
        texturePlaneCamera.GetComponent<Renderer>().material.mainTexture = mTexCamera;
    }

    /// <summary>
    /// 每帧更新方法（当前未使用）
    /// </summary>
    void Update()
    {
        // UpdateTexture();
    }


    /// <summary>
    /// 初始化纹理和像素数据，为与原生库交互做准备
    /// </summary>
    void InitTexture()
    {
        // 创建表情纹理
        mTexEmoji = new Texture2D(textureWidth, textureHeight, TextureFormat.ARGB32, false);
        mPixel32Emoji = mTexEmoji.GetPixels32();
        // 固定像素数组内存
        mPixelHandleEmoji = GCHandle.Alloc(mPixel32Emoji, GCHandleType.Pinned);
        // 获取固定内存地址
        mPixelPtrEmoji = mPixelHandleEmoji.AddrOfPinnedObject();

        // 创建摄像头纹理
        mTexCamera = new Texture2D(textureWidth, textureHeight, TextureFormat.ARGB32, false);
        mPixel32Camera = mTexCamera.GetPixels32();
        // 固定像素数组内存
        mPixelHandleCamera = GCHandle.Alloc(mPixel32Camera, GCHandleType.Pinned);
        // 获取固定内存地址
        mPixelPtrCamera = mPixelHandleCamera.AddrOfPinnedObject();
    }

    /// <summary>
    /// 关键帧变化更新，通知原生库当前帧的文件路径
    /// </summary>
    public void KeyFrameChangeUpdate()
    {
        string path = poseEditor.timelineFrames[robot.currentFrame].GetComponent<FrameMeta>().filePath;

        if (path.Length > 0)
        {
            // 通知原生库关键帧文件路径变化
            Native_OnKeyFrameChange(poseEditor.timelineFrames[robot.currentFrame]
                .GetComponent<FrameMeta>().filePath);

            Debug.Log(path);
        }
    }

    /// <summary>
    /// 固定频率更新方法，处理机器人关节数据同步和图像更新
    /// </summary>
    public void FixUpdate()
    {
        float[] joints = new float[6];
        // 模型优先模式：从UI滑块获取关节角度
        if (robot.syncMode == 0)
        {
            joints[0] = robot.sliderAngleHead.Value;
            joints[1] = robot.sliderAngleArmRollLeft.Value;
            joints[2] = robot.sliderAngleArmPitchLeft.Value;
            joints[3] = robot.sliderAngleArmRollRight.Value;
            joints[4] = robot.sliderAngleArmPitchRight.Value;
            joints[5] = robot.sliderAngleBody.Value;
        }

        // 调用原生库更新图像和关节数据
        IntPtr retArrayPtr = Native_OnFixUpdate(mPixelPtrEmoji, mPixelPtrCamera, textureWidth, textureHeight,
            Marshal.UnsafeAddrOfPinnedArrayElement(joints, 0), robot.syncMode == 0);
        float[] retJoints = new float[6];
        Marshal.Copy(retArrayPtr, retJoints, 0, 6);
        
        // 实体优先模式：从原生库获取关节角度并更新UI
        if (robot.syncMode == 2)
        {
            robot.targetAngleHead = retJoints[0];
            robot.targetAngleArmRollLeft = retJoints[1];
            robot.targetAngleArmPitchLeft = retJoints[2];
            robot.targetAngleArmRollRight = retJoints[3];
            robot.targetAngleArmPitchRight = retJoints[4];
            robot.targetAngleBody = retJoints[5];
            
            robot.sliderAngleHead.Value = (int) retJoints[0];
            robot.sliderAngleArmRollLeft.Value = (int) retJoints[1];
            robot.sliderAngleArmPitchLeft.Value = (int) retJoints[2];
            robot.sliderAngleArmRollRight.Value = (int) retJoints[3];
            robot.sliderAngleArmPitchRight.Value = (int) retJoints[4];
            robot.sliderAngleBody.Value = (int) retJoints[5];
            
            
            Debug.Log(retJoints[0]+" "+retJoints[1]+" "+retJoints[2]+" "+
                      retJoints[3]+" "+retJoints[4]+" "+retJoints[5]);
        }

        // 更新纹理显示
        mTexEmoji.SetPixels32(mPixel32Emoji);
        mTexEmoji.Apply();
        mTexCamera.SetPixels32(mPixel32Camera);
        mTexCamera.Apply();
    }


    /// <summary>
    /// 应用程序退出时的清理方法
    /// </summary>
    void OnApplicationQuit()
    {
        // 释放固定的内存句柄
        mPixelHandleEmoji.Free();
        mPixelHandleCamera.Free();
        // 通知原生库退出
        Native_OnExit();

        Debug.Log("OnApplicationQuit called");
    }

// #endif
}