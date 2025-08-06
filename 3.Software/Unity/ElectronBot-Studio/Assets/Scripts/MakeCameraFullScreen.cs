using System.Collections;
using System.Collections.Generic;
using UnityEngine;

/// <summary>
/// 摄像头全屏显示控制器
/// 负责处理摄像头预览的全屏适配，支持不同设备方向和屏幕比例
/// 主要用于Android平台的摄像头显示适配
/// </summary>
public class MakeCameraFullScreen : MonoBehaviour
{
    /// <summary>日志标签</summary>
    private const string TAG = "pzh::Unity::";

    /// <summary>原生视图摄像头</summary>
    public Camera NativeViewCamera;

    /// <summary>当前设备方向</summary>
    private DeviceOrientation currentOrientation;
    /// <summary>预览高度</summary>
    private int previewHeight;
    /// <summary>预览宽度</summary>
    private int previewWidth;
    /// <summary>屏幕高度</summary>
    private int screenHeight;
    /// <summary>屏幕宽度</summary>
    private int screenWidth;


    /// <summary>
    /// 初始化摄像头全屏显示设置
    /// 根据平台和屏幕方向配置摄像头参数
    /// </summary>
    void Awake()
    {
        // 防止屏幕休眠
        Screen.sleepTimeout = SleepTimeout.NeverSleep;

#if (UNITY_ANDROID && !UNITY_EDITOR)
        // Android平台获取原生摄像头控制器的预览尺寸
        previewHeight = this.GetComponent<NativeCameraController>().previewHeight;
        previewWidth = this.GetComponent<NativeCameraController>().previewWidth;
#else
        // 编辑器模式使用默认尺寸
        previewHeight = 480;
        previewWidth = 640;
#endif

        // 设置摄像头为正交投影
        NativeViewCamera.orthographic = true;

#if (UNITY_ANDROID && !UNITY_EDITOR)
        // 前置摄像头需要设置镜像
        if (GetComponent<NativeCameraController>().cameraID == 1)
            transform.Find("TexturePlane").transform.localScale = new Vector3(-1, 1, 1);

        // 横屏左旋转模式
        if (Screen.orientation == ScreenOrientation.LandscapeLeft)
        {
            currentOrientation = DeviceOrientation.LandscapeLeft;

            screenWidth = Screen.width;
            screenHeight = Screen.height;
            gameObject.transform.rotation = Quaternion.AngleAxis(0, new Vector3(0, 0, -1));

            // 设置正交摄像头大小
            NativeViewCamera.orthographicSize = screenHeight / 2 * 0.1f;

            // 根据屏幕和预览比例调整缩放
            if ((float)screenWidth / screenHeight > (float)previewWidth / previewHeight)
            {
                gameObject.transform.localScale = new Vector3(-screenWidth * 0.01f
                    , (float)screenWidth * previewHeight / previewWidth / screenHeight
                    * screenHeight * 0.01f, 1);
            }
            else
            {
                gameObject.transform.localScale = new Vector3(-screenWidth * 0.01f
                   * (float)screenHeight * previewWidth / previewHeight / screenWidth
                   , screenHeight * 0.01f, 1);
            }
        }
        // 竖屏模式
        else if (Screen.orientation == ScreenOrientation.Portrait)
        {
            currentOrientation = DeviceOrientation.Portrait;

            screenWidth = Screen.height;
            screenHeight = Screen.width;
            gameObject.transform.rotation = Quaternion.AngleAxis(90, new Vector3(0, 0, -1));

            // 设置正交摄像头大小
            NativeViewCamera.orthographicSize = screenWidth / 2 * 0.1f;

            // 根据屏幕和预览比例调整缩放
            if ((float)screenWidth / screenHeight > (float)previewWidth / previewHeight)
            {
                gameObject.transform.localScale = new Vector3(-screenWidth * 0.01f
                    , (float)screenWidth * previewHeight / previewWidth / screenHeight
                    * screenHeight * 0.01f, 1);
            }
            else
            {
                gameObject.transform.localScale = new Vector3(-screenWidth * 0.01f
                   * (float)screenHeight * previewWidth / previewHeight / screenWidth
                   , screenHeight * 0.01f, 1);
            }
        }
#else
        // 编辑器模式或非Android平台
        {
            currentOrientation = DeviceOrientation.LandscapeLeft;

            screenWidth = previewWidth; // 使用预览宽度
            screenHeight = previewHeight; // 使用预览高度
            gameObject.transform.rotation = Quaternion.AngleAxis(0, new Vector3(0, 0, -1));

            // 设置正交摄像头大小
            NativeViewCamera.orthographicSize = screenHeight / 2 * 0.1f;

            // 根据屏幕和预览比例调整缩放
            if ((float) screenWidth / screenHeight > (float) previewWidth / previewHeight)
            {
                gameObject.transform.localScale = new Vector3(-screenWidth * 0.01f
                    , (float) screenWidth * previewHeight / previewWidth / screenHeight
                      * screenHeight * 0.01f, 1);
            }
            else
            {
                gameObject.transform.localScale = new Vector3(-screenWidth * 0.01f
                                                                           * (float) screenHeight * previewWidth /
                                                              previewHeight / screenWidth
                    , screenHeight * 0.01f, 1);
            }
        }
#endif

        Debug.Log(TAG + "Screen width:" + screenWidth + " Screen height:" + screenHeight);
    }


    /// <summary>
    /// 启动时调用Android桥接器设置屏幕方向
    /// </summary>
    void Start()
    {
#if (UNITY_ANDROID && !UNITY_EDITOR)
        // 根据当前屏幕方向调用Android桥接器
        if (Screen.orientation == ScreenOrientation.LandscapeLeft)
            AndroidBridgeManager.GetComponent<AndroidBridge>().CallAndroidForBool("LandscapeLeft");
        else if (Screen.orientation == ScreenOrientation.Portrait)
            AndroidBridgeManager.GetComponent<AndroidBridge>().CallAndroidForBool("Portrait");
#endif
    }

    /// <summary>
    /// 每帧更新，监听设备方向变化并调整摄像头显示
    /// </summary>
    void Update()
    {
        // 检测设备方向变化
        if (Input.deviceOrientation != currentOrientation)
        {
            // 切换到横屏左旋转
            if (Input.deviceOrientation == DeviceOrientation.LandscapeLeft)
            {
                currentOrientation = DeviceOrientation.LandscapeLeft;

                gameObject.transform.rotation = Quaternion.AngleAxis(0, new Vector3(0, 0, -1));

                // 设置正交摄像头大小
                NativeViewCamera.orthographicSize = screenHeight / 2 * 0.1f;

                // 根据屏幕比例调整缩放
                if ((float) screenWidth / screenHeight > (float) previewWidth / previewHeight)
                {
                    gameObject.transform.localScale = new Vector3(-screenWidth * 0.01f
                        , (float) screenWidth * previewHeight / previewWidth / screenHeight
                          * screenHeight * 0.01f, 1);
                }
                else
                {
                    gameObject.transform.localScale = new Vector3(-screenWidth * 0.01f
                                                                               * (float) screenHeight * previewWidth /
                                                                  previewHeight / screenWidth
                        , screenHeight * 0.01f, 1);
                }

#if (UNITY_ANDROID && !UNITY_EDITOR)
                // 通知Android桥接器方向变化
                AndroidBridgeManager.GetComponent<AndroidBridge>().CallAndroidForBool("LandscapeLeft");
#endif
            }
            // 切换到竖屏模式
            else if (Input.deviceOrientation == DeviceOrientation.Portrait)
            {
                currentOrientation = DeviceOrientation.Portrait;

                gameObject.transform.rotation = Quaternion.AngleAxis(90, new Vector3(0, 0, -1));

                // 设置正交摄像头大小
                NativeViewCamera.orthographicSize = screenWidth / 2 * 0.1f;

                // 根据屏幕比例调整缩放
                if ((float) screenWidth / screenHeight > (float) previewWidth / previewHeight)
                {
                    gameObject.transform.localScale = new Vector3(-screenWidth * 0.01f
                        , (float) screenWidth * previewHeight / previewWidth / screenHeight
                          * screenHeight * 0.01f, 1);
                }
                else
                {
                    gameObject.transform.localScale = new Vector3(-screenWidth * 0.01f
                                                                               * (float) screenHeight * previewWidth /
                                                                  previewHeight / screenWidth
                        , screenHeight * 0.01f, 1);
                }

#if (UNITY_ANDROID && !UNITY_EDITOR)
                // 通知Android桥接器方向变化
                AndroidBridgeManager.GetComponent<AndroidBridge>().CallAndroidForBool("Portrait");
#endif
            }
        }

        // ESC键退出应用
        if (Input.GetKeyDown(KeyCode.Escape))
            Application.Quit();
    }
}