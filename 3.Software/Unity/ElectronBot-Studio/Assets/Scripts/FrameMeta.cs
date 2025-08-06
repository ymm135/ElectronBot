using System.Collections;
using System.Collections.Generic;
using UnityEngine;

/// <summary>
/// 帧元数据类
/// 存储单个动画帧的所有信息，包括机器人各关节角度、渲染纹理和关联文件路径
/// </summary>
public class FrameMeta : MonoBehaviour
{
    /// <summary>帧的唯一标识ID</summary>
    public int id;
    /// <summary>用于存储帧预览图像的渲染纹理</summary>
    public RenderTexture renderTexture;

    /// <summary>左臂滚转角度</summary>
    public float targetAngleArmRollLeft;
    /// <summary>左臂俯仰角度</summary>
    public float targetAngleArmPitchLeft;
    /// <summary>右臂滚转角度</summary>
    public float targetAngleArmRollRight;
    /// <summary>右臂俯仰角度</summary>
    public float targetAngleArmPitchRight;
    /// <summary>头部角度</summary>
    public float targetAngleHead;
    /// <summary>身体角度</summary>
    public float targetAngleBody;
    /// <summary>背光亮度</summary>
    public float backLight;

    /// <summary>关联的文件路径</summary>
    public string filePath;

    /// <summary>
    /// 初始化方法
    /// </summary>
    void Start()
    {
    }

    /// <summary>
    /// 每帧更新方法（当前为空实现）
    /// </summary>
    void Update()
    {
    }
}