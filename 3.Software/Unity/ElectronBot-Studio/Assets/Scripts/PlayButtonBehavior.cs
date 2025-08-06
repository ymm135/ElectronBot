using UnityEngine;
using UnityEngine.UI;
using UnityEngine.UI.ProceduralImage;
using UnityEngine.UIElements;
using Image = UnityEngine.UI.Image;

/// <summary>
/// 播放按钮行为控制器
/// 负责控制机器人动画的播放、暂停，以及时间轴的进度管理
/// </summary>
public class PlayButtonBehavior : MonoBehaviour
{
    /// <summary>播放状态的图标精灵</summary>
    public Sprite imgPlay;
    /// <summary>暂停状态的图标精灵</summary>
    public Sprite imgPause;
    /// <summary>时间轴滚动条组件</summary>
    public Scrollbar timelineSb;
    /// <summary>时间轴帧管理器对象</summary>
    public GameObject timelineFrameManager;
    /// <summary>机器人对象</summary>
    public GameObject robot;
    /// <summary>帧间时间间隔（秒）</summary>
    public float deltaTime = 0.1f;

    /// <summary>当前是否正在播放</summary>
    private bool isPlaying = false;

    /// <summary>
    /// 初始化方法
    /// </summary>
    void Start()
    {
    }

    /// <summary>
    /// 每帧更新播放逻辑，控制时间轴进度和机器人动画
    /// </summary>
    void Update()
    {
        if (isPlaying && timelineSb.GetComponent<Scrollbar>().value < 1)
        {
            int frameCount = timelineFrameManager.GetComponent<PoseEditor>().timelineFrames.Count;
            float totalTime = frameCount * deltaTime;

            timelineSb.GetComponent<Scrollbar>().value += (Time.deltaTime / totalTime);


            RobotController rc = robot.GetComponent<RobotController>();
            rc.slerpRatio = 0.01f / deltaTime; // need tuning
            int index = (int) (timelineSb.GetComponent<Scrollbar>().value * (frameCount - 1));
            FrameMeta meta = timelineFrameManager.GetComponent<PoseEditor>().timelineFrames[index]
                .GetComponent<FrameMeta>();
            rc.requestFrame = index;
            rc.targetAngleBody = meta.targetAngleBody;
            rc.targetAngleHead = meta.targetAngleHead;
            rc.targetAngleArmRollLeft = meta.targetAngleArmRollLeft;
            rc.targetAngleArmPitchLeft = meta.targetAngleArmPitchLeft;
            rc.targetAngleArmRollRight = meta.targetAngleArmRollRight;
            rc.targetAngleArmPitchRight = meta.targetAngleArmPitchRight;
            
            rc.isPlaying = true;
        }
        else
        {
            isPlaying = false;
            transform.Find("Icon").GetComponent<Image>().sprite = imgPlay;
            RobotController rc = robot.GetComponent<RobotController>();
            rc.slerpRatio = 0.5f * Time.deltaTime / 0.03f; // need tuning
            rc.isPlaying = false;
        }
    }

    /// <summary>
    /// 播放按钮点击事件处理
    /// 切换播放/暂停状态，更新按钮图标和机器人控制器状态
    /// </summary>
    public void OnClick()
    {
        isPlaying = !isPlaying;
        RobotController rc = robot.GetComponent<RobotController>();
        rc.isPlaying = isPlaying;

        if (isPlaying)
        {
            // 开始播放：切换到暂停图标，重置时间轴到起始位置
            transform.Find("Icon").GetComponent<Image>().sprite = imgPause;
            timelineSb.GetComponent<Scrollbar>().value = 0;
        }
        else
        {
            // 暂停播放：切换到播放图标
            transform.Find("Icon").GetComponent<Image>().sprite = imgPlay;
        }
    }

    /// <summary>
    /// 时间间隔改变事件处理（调试用）
    /// </summary>
    /// <param name="_val">新的时间间隔值</param>
    public void OnDeltaTimeChanged(float _val)
    {
        Debug.Log(_val);
    }
}