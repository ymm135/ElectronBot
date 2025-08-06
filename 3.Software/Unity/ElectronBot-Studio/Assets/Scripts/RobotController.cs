using System;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UIWidgets;
using UnityEngine;
using UnityEngine.UI;

/// <summary>
/// ElectronBot机器人控制器
/// 负责控制机器人的各个关节角度、姿态同步和UI交互
/// </summary>
public class RobotController : MonoBehaviour
{
    /// <summary>插值比例，控制机器人动作的平滑度</summary>
    public float slerpRatio = 0.5f;
    /// <summary>同步模式：0-模型优先，1-禁用同步，2-实体优先</summary>
    public int syncMode = 1;

    /// <summary>左臂滚转关节Transform</summary>
    public Transform armRollLeft;
    /// <summary>左臂俯仰关节Transform</summary>
    public Transform armPitchLeft;
    /// <summary>右臂滚转关节Transform</summary>
    public Transform armRollRight;
    /// <summary>右臂俯仰关节Transform</summary>
    public Transform armPitchRight;
    /// <summary>头部关节Transform</summary>
    public Transform head;
    /// <summary>身体关节Transform</summary>
    public Transform body;

    /// <summary>左臂滚转目标角度</summary>
    public float targetAngleArmRollLeft;
    /// <summary>左臂俯仰目标角度</summary>
    public float targetAngleArmPitchLeft;
    /// <summary>右臂滚转目标角度</summary>
    public float targetAngleArmRollRight;
    /// <summary>右臂俯仰目标角度</summary>
    public float targetAngleArmPitchRight;
    /// <summary>头部目标角度</summary>
    public float targetAngleHead;
    /// <summary>身体目标角度</summary>
    public float targetAngleBody;
    /// <summary>背光亮度</summary>
    public float backLight;


    /// <summary>左臂滚转角度滑块</summary>
    public CenteredSlider sliderAngleArmRollLeft;
    /// <summary>左臂俯仰角度滑块</summary>
    public CenteredSlider sliderAngleArmPitchLeft;
    /// <summary>右臂滚转角度滑块</summary>
    public CenteredSlider sliderAngleArmRollRight;
    /// <summary>右臂俯仰角度滑块</summary>
    public CenteredSlider sliderAngleArmPitchRight;
    /// <summary>头部角度滑块</summary>
    public CenteredSlider sliderAngleHead;
    /// <summary>身体角度滑块</summary>
    public CenteredSlider sliderAngleBody;
    /// <summary>滑块遮罩层</summary>
    public Transform sliderCover;

    /// <summary>请求的帧号</summary>
    public int requestFrame = -1;
    /// <summary>当前帧号</summary>
    public int currentFrame = -1;
    /// <summary>是否正在播放动画</summary>
    public bool isPlaying = false;

    /// <summary>计算机视觉管理器</summary>
    public UnityGetImageFromCpp cvManager;


    /// <summary>
    /// 初始化机器人控制器，设置所有关节的初始角度
    /// </summary>
    void Start()
    {
        targetAngleArmRollLeft = 0;
        targetAngleArmPitchLeft = 0;
        targetAngleArmRollRight = 0;
        targetAngleArmPitchRight = 0;
        targetAngleHead = 0;
        targetAngleBody = 0;
        backLight = 1;
    }

    /// <summary>
    /// 每帧更新机器人关节角度，使用插值实现平滑动作
    /// 同时在播放模式下同步滑块UI显示
    /// </summary>
    void Update()
    {
        armRollLeft.localRotation = Quaternion.Slerp(armRollLeft.localRotation,
            Quaternion.Euler(0, 0, targetAngleArmRollLeft), slerpRatio);
        armPitchLeft.localRotation = Quaternion.Slerp(armPitchLeft.localRotation,
            Quaternion.Euler(targetAngleArmPitchLeft, 0, 0), slerpRatio);
        armRollRight.localRotation = Quaternion.Slerp(armRollRight.localRotation,
            Quaternion.Euler(0, 0, -targetAngleArmRollRight), slerpRatio);
        armPitchRight.localRotation = Quaternion.Slerp(armPitchRight.localRotation,
            Quaternion.Euler(targetAngleArmPitchRight, 0, 0), slerpRatio);
        body.localRotation = Quaternion.Slerp(body.localRotation,
            Quaternion.Euler(0, targetAngleBody, 0), slerpRatio);
        head.localRotation = Quaternion.Slerp(head.localRotation,
            Quaternion.Euler(targetAngleHead, 0, 0), slerpRatio);

        if (isPlaying)
        {
            sliderAngleBody.Value = (int) targetAngleBody;
            sliderAngleHead.Value = (int) targetAngleHead;
            sliderAngleArmRollLeft.Value = (int) targetAngleArmRollLeft;
            sliderAngleArmPitchLeft.Value = (int) targetAngleArmPitchLeft;
            sliderAngleArmRollRight.Value = (int) targetAngleArmRollRight;
            sliderAngleArmPitchRight.Value = (int) targetAngleArmPitchRight;
        }
    }

    /// <summary>
    /// 固定频率更新（20Hz），处理计算机视觉和时间轴相关逻辑
    /// </summary>
    private void FixedUpdate()
    {
        // 20Hz频率更新计算机视觉管理器
        cvManager.FixUpdate();

        // 处理时间轴播放逻辑
        if (isPlaying && requestFrame != currentFrame)
        {
            currentFrame = requestFrame;
            cvManager.KeyFrameChangeUpdate();

            Debug.Log(">>>> " + currentFrame);
        }
    }

    /// <summary>
    /// 设置左臂滚转角度
    /// </summary>
    /// <param name="_val">目标角度值</param>
    public void SetAngleArmRollLeft(int _val)
    {
        targetAngleArmRollLeft = _val;
    }

    /// <summary>
    /// 设置左臂俯仰角度
    /// </summary>
    /// <param name="_val">目标角度值</param>
    public void SetAngleArmPitchLeft(int _val)
    {
        targetAngleArmPitchLeft = _val;
    }

    /// <summary>
    /// 设置右臂滚转角度
    /// </summary>
    /// <param name="_val">目标角度值</param>
    public void SetAngleArmRollRight(int _val)
    {
        targetAngleArmRollRight = _val;
    }

    /// <summary>
    /// 设置右臂俯仰角度
    /// </summary>
    /// <param name="_val">目标角度值</param>
    public void SetAngleArmPitchRight(int _val)
    {
        targetAngleArmPitchRight = _val;
    }

    /// <summary>
    /// 设置身体旋转角度
    /// </summary>
    /// <param name="_val">目标角度值</param>
    public void SetAngleBody(int _val)
    {
        targetAngleBody = _val;
    }

    /// <summary>
    /// 设置头部角度
    /// </summary>
    /// <param name="_val">目标角度值</param>
    public void SetAngleHead(int _val)
    {
        targetAngleHead = _val;
    }


    /// <summary>
    /// 重置机器人姿态到初始状态
    /// 将所有关节角度和UI滑块都设置为0
    /// </summary>
    public void ResetPose()
    {
        targetAngleArmRollLeft = 0;
        targetAngleArmPitchLeft = 0;
        targetAngleArmRollRight = 0;
        targetAngleArmPitchRight = 0;
        targetAngleHead = 0;
        targetAngleBody = 0;

        sliderAngleArmRollLeft.Value = 0;
        sliderAngleArmPitchLeft.Value = 0;
        sliderAngleArmRollRight.Value = 0;
        sliderAngleArmPitchRight.Value = 0;
        sliderAngleHead.Value = 0;
        sliderAngleBody.Value = 0;
    }


    /// <summary>
    /// 同步模式改变时的回调函数
    /// 根据不同模式更新UI显示和控制逻辑
    /// </summary>
    /// <param name="_slider">触发事件的滑块</param>
    public void OnSyncModeChanged(Slider _slider)
    {
        syncMode = (int) _slider.value;
        switch (syncMode)
        {
            case 0: // 模型优先模式
                _slider.transform.Find("Text (TMP)").GetComponent<TMP_Text>().text = "模型优先";
                sliderCover.gameObject.SetActive(false);
                break;
            case 1: // 禁用同步模式
                _slider.transform.Find("Text (TMP)").GetComponent<TMP_Text>().text = "禁用同步";
                sliderCover.gameObject.SetActive(false);
                break;
            case 2: // 实体优先模式
                _slider.transform.Find("Text (TMP)").GetComponent<TMP_Text>().text = "实体优先";
                sliderCover.gameObject.SetActive(true);
                break;
        }
    }
}