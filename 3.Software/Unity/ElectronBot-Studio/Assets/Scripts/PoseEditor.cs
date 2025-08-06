using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using TMPro;
using UnityEngine;
using UnityEngine.Serialization;
using UnityEngine.UI;

/// <summary>
/// 姿态编辑器
/// 负责管理机器人动画时间轴，包括帧的添加、删除、修改和选择
/// 支持帧预览图像捕获和文件路径选择功能
/// </summary>
public class PoseEditor : MonoBehaviour
{
    /// <summary>帧预制体模板</summary>
    public GameObject framePrefab;
    /// <summary>时间轴帧列表</summary>
    [FormerlySerializedAs("timeline")] public List<GameObject> timelineFrames;
    /// <summary>用于渲染帧预览的摄像机</summary>
    public Camera renderCamera;
    /// <summary>机器人对象引用</summary>
    public GameObject robot;

    /// <summary>
    /// 初始化方法，创建第一个帧
    /// </summary>
    void Start()
    {
        GameObject frame0 = Instantiate(framePrefab, transform, true);
        SetupFrame(frame0, 0);
        frame0.transform.Find("FrameAdd").Find("Text (TMP)").GetComponent<TMP_Text>().text = "+";
        timelineFrames.Add(frame0);
        SetFrameCapture(0);
    }

    /// <summary>
    /// 每帧更新方法（当前为空实现）
    /// </summary>
    void Update()
    {
    }


    /// <summary>
    /// 添加新帧的回调方法
    /// </summary>
    /// <param name="_id">在指定ID后添加新帧</param>
    public void AddFrameCallback(int _id)
    {
        GameObject frame = Instantiate(framePrefab, transform, true);
        _id += 1;
        SetupFrame(frame, _id);
        timelineFrames.Insert(frame.GetComponent<FrameMeta>().id, frame);
        SetFrameCapture(_id);

        // 重新设置后续帧的ID
        if (frame.GetComponent<FrameMeta>().id < timelineFrames.Count - 1)
            for (int i = frame.GetComponent<FrameMeta>().id + 1; i < timelineFrames.Count; i++)
            {
                GameObject f = timelineFrames[i];
                SetupFrame(f, i);
            }

        // 调整时间轴容器大小
        GetComponent<RectTransform>().sizeDelta = new Vector2(820 + 160 * timelineFrames.Count, 140.0f);
    }

    /// <summary>
    /// 修改帧的回调方法，重新捕获当前机器人状态
    /// </summary>
    /// <param name="_id">要修改的帧ID</param>
    public void ModifyFrameCallback(int _id)
    {
        SetFrameCapture(_id);
    }

    /// <summary>
    /// 选择帧的回调方法，将帧的姿态数据应用到机器人和UI滑块
    /// </summary>
    /// <param name="_id">要选择的帧ID</param>
    public void SelectFrameCallback(int _id)
    {
        GameObject frame = timelineFrames[_id];
        var rc = robot.GetComponent<RobotController>();

        // 设置机器人目标角度
        rc.targetAngleBody = frame.GetComponent<FrameMeta>().targetAngleBody;
        rc.targetAngleHead = frame.GetComponent<FrameMeta>().targetAngleHead;
        rc.targetAngleArmPitchLeft = frame.GetComponent<FrameMeta>().targetAngleArmPitchLeft;
        rc.targetAngleArmRollLeft = frame.GetComponent<FrameMeta>().targetAngleArmRollLeft;
        rc.targetAngleArmPitchRight = frame.GetComponent<FrameMeta>().targetAngleArmPitchRight;
        rc.targetAngleArmRollRight = frame.GetComponent<FrameMeta>().targetAngleArmRollRight;
        
        // 同步更新UI滑块值
        rc.sliderAngleBody.Value = (int) rc.targetAngleBody;
        rc.sliderAngleHead.Value = (int) rc.targetAngleHead;
        rc.sliderAngleArmPitchLeft.Value = (int) rc.targetAngleArmPitchLeft;
        rc.sliderAngleArmRollLeft.Value = (int) rc.targetAngleArmRollLeft;
        rc.sliderAngleArmPitchRight.Value = (int) rc.targetAngleArmPitchRight;
        rc.sliderAngleArmRollRight.Value = (int) rc.targetAngleArmRollRight;
    }


    /// <summary>
    /// 删除帧的回调方法
    /// </summary>
    /// <param name="_id">要删除的帧ID（不能删除第0帧）</param>
    public void DeleteFrameCallback(int _id)
    {
        if (_id > 0)
        {
            Destroy(timelineFrames[_id]);
            timelineFrames.RemoveAt(_id);

            // 重新设置后续帧的ID
            for (int i = _id; i < timelineFrames.Count; i++)
            {
                GameObject f = timelineFrames[i];
                SetupFrame(f, i);
            }

            // 调整时间轴容器大小
            GetComponent<RectTransform>().sizeDelta = new Vector2(820 + 160 * timelineFrames.Count, 140.0f);
        }
    }


    /// <summary>
    /// 设置帧的基本属性和事件监听器
    /// </summary>
    /// <param name="frame">要设置的帧对象</param>
    /// <param name="_id">帧的ID</param>
    private void SetupFrame(GameObject frame, int _id)
    {
        frame.GetComponent<FrameMeta>().id = _id;
        frame.name = "Frame_" + _id;
        // 设置帧在时间轴上的位置
        frame.transform.GetComponent<RectTransform>().anchoredPosition3D =
            new Vector3(91 + 160 * _id, 0, 0);
        
        // 设置添加帧按钮事件
        frame.transform.Find("FrameAdd").GetComponent<Button>().onClick.RemoveAllListeners();
        frame.transform.Find("FrameAdd").GetComponent<Button>().onClick
            .AddListener(() => AddFrameCallback(frame.GetComponent<FrameMeta>().id));
        
        // 设置捕获按钮事件
        frame.transform.Find("FrameView").Find("Capture").GetComponent<Button>().onClick.RemoveAllListeners();
        frame.transform.Find("FrameView").Find("Capture").GetComponent<Button>().onClick
            .AddListener(() => ModifyFrameCallback(frame.GetComponent<FrameMeta>().id));
        
        // 设置帧选择事件
        frame.transform.Find("FrameView").GetComponent<Button>().onClick.RemoveAllListeners();
        frame.transform.Find("FrameView").GetComponent<Button>().onClick
            .AddListener(() => SelectFrameCallback(frame.GetComponent<FrameMeta>().id));
        
        // 设置删除帧按钮事件
        frame.transform.Find("FrameDelete").GetComponent<Button>().onClick.RemoveAllListeners();
        frame.transform.Find("FrameDelete").GetComponent<Button>().onClick
            .AddListener(() => DeleteFrameCallback(frame.GetComponent<FrameMeta>().id));
        
        // 设置文件路径按钮事件
        frame.transform.Find("FilePath").GetComponent<Button>().onClick.RemoveAllListeners();
        frame.transform.Find("FilePath").GetComponent<Button>().onClick
            .AddListener(() => FilePathCallback(frame.GetComponent<FrameMeta>().id));

        // 设置帧编号显示
        frame.transform.Find("FrameAdd").Find("Text (TMP)").GetComponent<TMP_Text>().text = "" + _id;
    }

    /// <summary>
    /// 捕获当前机器人状态并保存到指定帧
    /// </summary>
    /// <param name="_id">要捕获状态的帧ID</param>
    private void SetFrameCapture(int _id)
    {
        GameObject frame = timelineFrames[_id];

        // 捕获摄像机画面作为帧预览
        frame.transform.Find("FrameView").GetComponent<RawImage>().texture =
            CaptureCamera(frame.GetComponent<FrameMeta>().renderTexture);

        // 保存当前机器人的所有关节角度
        frame.GetComponent<FrameMeta>().targetAngleBody =
            robot.GetComponent<RobotController>().targetAngleBody;
        frame.GetComponent<FrameMeta>().targetAngleHead =
            robot.GetComponent<RobotController>().targetAngleHead;
        frame.GetComponent<FrameMeta>().targetAngleArmPitchLeft =
            robot.GetComponent<RobotController>().targetAngleArmPitchLeft;
        frame.GetComponent<FrameMeta>().targetAngleArmRollLeft =
            robot.GetComponent<RobotController>().targetAngleArmRollLeft;
        frame.GetComponent<FrameMeta>().targetAngleArmPitchRight =
            robot.GetComponent<RobotController>().targetAngleArmPitchRight;
        frame.GetComponent<FrameMeta>().targetAngleArmRollRight =
            robot.GetComponent<RobotController>().targetAngleArmRollRight;
    }


    /// <summary>
    /// 文件路径选择回调方法，打开文件选择对话框
    /// </summary>
    /// <param name="_id">要设置文件路径的帧ID</param>
    public void FilePathCallback(int _id)
    {
        var openFileName = new OpenFileName();
        openFileName.structSize = Marshal.SizeOf(openFileName);
        openFileName.filter = "文件(*.jpg;*.png;*.bmp;*.mp4;)\0*.jpg;*.png;*.bmp;*.mp4";
        openFileName.file = new string(new char[256]);
        openFileName.maxFile = openFileName.file.Length;
        openFileName.fileTitle = new string(new char[64]);
        openFileName.maxFileTitle = openFileName.fileTitle.Length;
        openFileName.initialDir = Application.streamingAssetsPath.Replace('/', '\\');
        openFileName.title = "选择文件";
        openFileName.flags = 0x00080000 | 0x00001000 | 0x00000800 | 0x00000008;

        if (LocalDialog.GetSaveFileName(openFileName))
        {
            Debug.Log(openFileName.file);
            GameObject frame = timelineFrames[_id];
            frame.GetComponent<FrameMeta>().filePath = openFileName.file;
            // 提取文件名并显示在UI上
            var splitFilePath = openFileName.file.Split('\\');
            frame.transform.Find("FilePath").Find("Text").GetComponent<Text>().text =
                splitFilePath[splitFilePath.Length - 1];
        }
    }

    /// <summary>
    /// 使用摄像机捕获当前画面到渲染纹理
    /// </summary>
    /// <param name="_rt">要更新的渲染纹理</param>
    /// <returns>捕获的渲染纹理</returns>
    private RenderTexture CaptureCamera(RenderTexture _rt)
    {
        if (_rt != null)
        {
            RenderTexture.ReleaseTemporary(_rt);
        }

        int rtW = 270;
        int rtH = 231;

        _rt = RenderTexture.GetTemporary(rtW, rtH, -5);
        renderCamera.targetTexture = _rt;
        renderCamera.Render();
        renderCamera.targetTexture = null;

        return _rt;
    }
}


/// <summary>
/// Windows文件对话框结构体
/// 参考：https://blog.csdn.net/pq8888168/article/details/85781908
/// </summary>
[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
public class OpenFileName
{
    public int structSize = 0;
    public IntPtr dlgOwner = IntPtr.Zero;
    public IntPtr instance = IntPtr.Zero;
    public String filter = null;
    public String customFilter = null;
    public int maxCustFilter = 0;
    public int filterIndex = 0;
    public String file = null;
    public int maxFile = 0;
    public String fileTitle = null;
    public int maxFileTitle = 0;
    public String initialDir = null;
    public String title = null;
    public int flags = 0;
    public short fileOffset = 0;
    public short fileExtension = 0;
    public String defExt = null;
    public IntPtr custData = IntPtr.Zero;
    public IntPtr hook = IntPtr.Zero;
    public String templateName = null;
    public IntPtr reservedPtr = IntPtr.Zero;
    public int reservedInt = 0;
    public int flagsEx = 0;
}

/// <summary>
/// 本地文件对话框工具类
/// 提供Windows系统文件选择对话框的调用接口
/// </summary>
public class LocalDialog
{
    /// <summary>
    /// 调用Windows打开文件对话框
    /// </summary>
    [DllImport("Comdlg32.dll", SetLastError = true, ThrowOnUnmappableChar = true, CharSet = CharSet.Auto)]
    public static extern bool GetOpenFileName([In, Out] OpenFileName ofn);

    /// <summary>
    /// 执行打开文件操作的封装方法
    /// </summary>
    public static bool GetOFN([In, Out] OpenFileName ofn)
    {
        return GetOpenFileName(ofn); //执行打开文件的操作
    }

    /// <summary>
    /// 调用Windows保存文件对话框
    /// </summary>
    [DllImport("Comdlg32.dll", SetLastError = true, ThrowOnUnmappableChar = true, CharSet = CharSet.Auto)]
    public static extern bool GetSaveFileName([In, Out] OpenFileName ofn);

    /// <summary>
    /// 执行保存文件操作的封装方法
    /// </summary>
    public static bool GetSFN([In, Out] OpenFileName ofn)
    {
        return GetSaveFileName(ofn); //执行保存选中文件的操作
    }
}