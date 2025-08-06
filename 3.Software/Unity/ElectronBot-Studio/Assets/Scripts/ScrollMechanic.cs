using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using TMPro;
using UnityEngine.EventSystems;
using System;

/// <summary>
/// 高级滚动机制组件
/// 提供带有动画效果、惯性滚动、无限滚动等功能的自定义滚动视图
/// 支持鼠标滚轮、触摸板和拖拽操作
/// </summary>
public class ScrollMechanic : MonoBehaviour, IDropHandler, IDragHandler, IBeginDragHandler, IPointerExitHandler,
    IPointerEnterHandler
{
    /// <summary>显示时间差的游戏对象</summary>
    public GameObject deltaTime;

    [Header("Test variables")] 
    /// <summary>测试初始化标志</summary>
    public bool initTest;
    /// <summary>是否启用无限滚动（需要初始化）</summary>
    public bool isInfinite;
    /// <summary>测试数据数组</summary>
    public string[] testData;

    [Header("Text prefab")] 
    /// <summary>文本模板预制体</summary>
    public GameObject templateValues;

    [Header("Required objects")] 
    /// <summary>主摄像机</summary>
    public new Camera camera;
    /// <summary>目标画布</summary>
    public RectTransform targetCanvas;
    /// <summary>内容目标容器</summary>
    public RectTransform contentTarget;
    /// <summary>自定义布局组件，可替代默认布局组</summary>
    public AutoSizeLayout contentSize;

    [Header("Settings")] [Space(20)] 
    /// <summary>模板文本矩形的高度</summary>
    public float heightTemplate = 27;
    /// <summary>控制滚动"形状"的动画曲线</summary>
    public AnimationCurve curve;
    /// <summary>控制文本偏移的动画曲线</summary>
    public AnimationCurve curveShift;
    /// <summary>聚焦速度</summary>
    public float speedLerp = 5;
    /// <summary>开始聚焦的最小惯性值</summary>
    public float minVelocity = 0.2f;
    /// <summary>上方文本的偏移量</summary>
    public float shiftUp = 32;
    /// <summary>下方文本的偏移量</summary>
    public float shiftDown = 32;
    /// <summary>上下边界的间距</summary>
    public float padding = 0;
    /// <summary>文本颜色的填充值</summary>
    [Range(0, 1)] public float colorPad = 0.115f;
    /// <summary>最大字体大小</summary>
    public float maxFontSize = 48.2f;
    /// <summary>是否启用弹性移动</summary>
    public bool isElastic = true;
    /// <summary>最大弹性距离</summary>
    public float maxElastic = 50;
    /// <summary>惯性敏感度</summary>
    public float inertiaSense = 4;

    [Header("Mouse Wheel and Touchpad scroll methods")]
    /// <summary>是否可以使用鼠标滚轮</summary>
    public bool isCanUseMouseWheel;
    /// <summary>是否反转鼠标滚轮方向</summary>
    public bool isInvertMouseWheel;
    /// <summary>鼠标滚轮敏感度</summary>
    public float mouseWheelSensibility = 0.5f;
    /// <summary>触摸板敏感度</summary>
    public float touchpadSensibility = 0.5f;

    /// <summary>是否正在拖拽</summary>
    bool isDragging;
    /// <summary>惯性值</summary>
    float inertia;
    /// <summary>内容起始位置</summary>
    float startPosContent;
    /// <summary>鼠标起始位置</summary>
    float startPosMouse;
    /// <summary>中心位置</summary>
    float middle;
    /// <summary>文本高度</summary>
    float heightText = 27;
    /// <summary>检查计数</summary>
    int countCheck = 4;
    /// <summary>当前中心索引</summary>
    int currentCenter;
    /// <summary>是否已初始化</summary>
    bool isInitialized;
    /// <summary>总数量</summary>
    int countTotal;
    /// <summary>填充计数</summary>
    int padCount;
    /// <summary>触摸板滚动值</summary>
    float _padScroll;

    public float MouseScroll
    {
        get
        {
            float mouseScroll = Input.mouseScrollDelta.y;

            if (mouseScroll != 0)
                return mouseScroll;
            else
                return _padScroll;
        }
    }

    //Get TrackPad Scroll
    void OnGUI()
    {
        if (Event.current.type == EventType.ScrollWheel)
            _padScroll = (-Event.current.delta.y / 10) * touchpadSensibility;
        else
            _padScroll = 0;
    }

    private void Start()
    {
        heightText = heightTemplate / 2;
        middle = GetComponent<RectTransform>().sizeDelta.y / 2;
        contentSize.topPad = middle - heightText;
        contentSize.bottomPad = middle - heightText;
        countCheck = Mathf.CeilToInt((middle * 2) / heightTemplate);
    }

    /// <summary>
    /// Initialization method
    /// </summary>
    /// <param name="dataToInit"> List of texts to show </param>
    /// <param name="isInfinite"> Is scroll will be infinite </param>
    /// <param name="firstTarget"> Which text in list will be first </param>
    public void Initialize(List<string> dataToInit, bool isInfinite = false, int firstTarget = 0)
    {
        countTotal = dataToInit.Count;
        for (int i = 0; i < contentTarget.childCount; i++)
        {
            Destroy(contentTarget.GetChild(i).gameObject);
        }

        this.isInfinite = isInfinite;

        if (isInfinite)
        {
            int half = (int) (countCheck / 2) + 1;

            if (dataToInit.Count > half)
            {
                padCount = half;
                for (int i = dataToInit.Count - half; i < dataToInit.Count; i++)
                {
                    var textComponent = Instantiate(templateValues, contentTarget.transform).transform.GetChild(0)
                        .GetComponent<TextMeshProUGUI>();
                    textComponent.text = dataToInit[i];
                    textComponent.transform.parent.name = i + "";
                    textComponent.transform.parent.GetComponent<RectTransform>().sizeDelta =
                        new Vector2(GetComponent<RectTransform>().sizeDelta.x, heightTemplate);
                }
            }
            else
            {
                padCount = dataToInit.Count;
                for (int j = 0; j < Mathf.CeilToInt((float) half / (float) dataToInit.Count); j++)
                {
                    for (int i = 0; i < dataToInit.Count; i++)
                    {
                        var textComponent = Instantiate(templateValues, contentTarget.transform).transform.GetChild(0)
                            .GetComponent<TextMeshProUGUI>();
                        textComponent.text = dataToInit[i];
                        textComponent.transform.parent.name = i + "";
                        textComponent.transform.parent.GetComponent<RectTransform>().sizeDelta =
                            new Vector2(GetComponent<RectTransform>().sizeDelta.x, heightTemplate);
                    }
                }
            }

            isElastic = false;
            contentTarget.anchoredPosition = new Vector2(0, (firstTarget + padCount) * (heightText * 2));
        }
        else
        {
            padCount = (int) (countCheck / 2) + 1;
            contentTarget.anchoredPosition = new Vector2(0, firstTarget * (heightText * 2));
        }

        for (int i = 0; i < dataToInit.Count; i++)
        {
            var textComponent = Instantiate(templateValues, contentTarget.transform).transform.GetChild(0)
                .GetComponent<TextMeshProUGUI>();
            textComponent.text = dataToInit[i];
            textComponent.transform.parent.name = i + "";
            textComponent.transform.parent.GetComponent<RectTransform>().sizeDelta =
                new Vector2(GetComponent<RectTransform>().sizeDelta.x, heightTemplate);
        }

        if (isInfinite)
        {
            int half = (int) (countCheck / 2) + 1;
            if (dataToInit.Count > half)
            {
                for (int i = 0; i < half; i++)
                {
                    var textComponent = Instantiate(templateValues, contentTarget.transform).transform.GetChild(0)
                        .GetComponent<TextMeshProUGUI>();
                    textComponent.text = dataToInit[i];
                    textComponent.transform.parent.name = i + "";
                    textComponent.transform.parent.GetComponent<RectTransform>().sizeDelta =
                        new Vector2(GetComponent<RectTransform>().sizeDelta.x, heightTemplate);
                }
            }
            else
            {
                for (int j = 0; j < Mathf.CeilToInt((float) half / (float) dataToInit.Count); j++)
                {
                    for (int i = 0; i < dataToInit.Count; i++)
                    {
                        var textComponent = Instantiate(templateValues, contentTarget.transform).transform.GetChild(0)
                            .GetComponent<TextMeshProUGUI>();
                        textComponent.text = dataToInit[i];
                        textComponent.transform.parent.name = i + "";
                        textComponent.transform.parent.GetComponent<RectTransform>().sizeDelta =
                            new Vector2(GetComponent<RectTransform>().sizeDelta.x, heightTemplate);
                    }
                }
            }
        }

        contentSize.UpdateLayout();
        isInitialized = true;
    }

    /// <summary>
    /// 返回当前聚焦项的列表ID
    /// </summary>
    /// <returns>当前聚焦项的索引</returns>
    public int GetCurrentValue()
    {
        return int.Parse(contentTarget.GetChild(currentCenter).name);
    }

    /// <summary>
    /// 每帧更新滚动逻辑，处理拖拽、惯性、边界检测和视觉效果
    /// </summary>
    private void Update()
    {
        if (Input.GetMouseButtonUp(0))
        {
            isDragging = false;
        }

        if (isCanUseMouseWheel && isInArea && Input.mouseScrollDelta.y != 0)
        {
            isDragging = true;
        }
        else if (!Input.GetMouseButton(0))
        {
            isDragging = false;
        }

        if (initTest)
        {
            initTest = false;
            var newList = new List<string>();
            for (int i = 0; i < testData.Length; i++)
            {
                newList.Add(testData[i]);
            }

            Initialize(newList, isInfinite);
        }

        if (isInitialized)
        {
            if (!isDragging)
            {
                if (contentTarget.anchoredPosition.y + inertia < 0)
                {
                    if (isElastic)
                    {
                        contentTarget.anchoredPosition = new Vector2(0, contentTarget.anchoredPosition.y + inertia);
                        inertia = inertia * Mathf.Clamp(1 - Mathf.Abs(contentTarget.anchoredPosition.y) /
                            maxElastic, 0, 1);
                    }
                    else
                    {
                        contentTarget.anchoredPosition = new Vector2(0, 0);
                        inertia = 0;
                    }
                }
                else if (contentTarget.anchoredPosition.y + inertia > contentTarget.sizeDelta.y - middle * 2)
                {
                    if (isElastic)
                    {
                        contentTarget.anchoredPosition = new Vector2(0, contentTarget.anchoredPosition.y + inertia);
                        inertia = inertia * Mathf.Clamp(1 - Mathf.Abs((contentTarget.sizeDelta.y - middle * 2) -
                                                                      contentTarget.anchoredPosition.y) /
                            maxElastic, 0, 1);
                    }
                    else
                    {
                        contentTarget.anchoredPosition = new Vector2(0, contentTarget.sizeDelta.y - middle * 2);
                        inertia = 0;
                    }
                }
                else
                {
                    contentTarget.anchoredPosition = new Vector2(0, contentTarget.anchoredPosition.y + inertia);
                    inertia = Mathf.Lerp(inertia, 0, inertiaSense * Time.deltaTime);
                }
            }
            else
            {
                if (isCanUseMouseWheel && isInArea && MouseScroll != 0)
                {
                    if (isElastic)
                    {
                        if (contentTarget.anchoredPosition.y < 0)
                        {
                            inertia = 0;
                            contentTarget.anchoredPosition = new Vector2(0,
                                contentTarget.anchoredPosition.y +
                                ((isInvertMouseWheel ? -1 : 1) * MouseScroll * mouseWheelSensibility)
                                * Mathf.Clamp(1 - Mathf.Abs(contentTarget.anchoredPosition.y) /
                                    maxElastic, 0, 1));
                        }
                        else if (contentTarget.anchoredPosition.y > contentTarget.sizeDelta.y - middle * 2)
                        {
                            inertia = 0;
                            contentTarget.anchoredPosition = new Vector2(0,
                                contentTarget.anchoredPosition.y +
                                ((isInvertMouseWheel ? -1 : 1) * MouseScroll * mouseWheelSensibility)
                                * Mathf.Clamp(1 - Mathf.Abs((contentTarget.sizeDelta.y - middle * 2) -
                                                            contentTarget.anchoredPosition.y) /
                                    maxElastic, 0, 1));
                        }
                        else
                        {
                            inertia += ((isInvertMouseWheel ? -1 : 1) * MouseScroll
                                                                      * mouseWheelSensibility);
                            contentTarget.anchoredPosition = new Vector2(0,
                                contentTarget.anchoredPosition.y +
                                ((isInvertMouseWheel ? -1 : 1) * MouseScroll * mouseWheelSensibility));
                        }
                    }
                    else
                    {
                        inertia += ((isInvertMouseWheel ? -1 : 1) * MouseScroll
                                                                  * mouseWheelSensibility);
                        contentTarget.anchoredPosition = new Vector2(0, Mathf.Clamp(
                            contentTarget.anchoredPosition.y +
                            ((isInvertMouseWheel ? -1 : 1) * MouseScroll * mouseWheelSensibility),
                            0, contentTarget.sizeDelta.y - middle * 2));
                    }
                }
                else
                {
                    if (isElastic)
                    {
                        if (contentTarget.anchoredPosition.y < 0)
                        {
                            inertia = 0;
                            contentTarget.anchoredPosition = new Vector2(0,
                                startPosContent + (-startPosMouse + (Input.mousePosition.y / camera.pixelHeight)
                                    * targetCanvas.sizeDelta.y) * Mathf.Clamp(1 -
                                                                              Mathf.Abs(
                                                                                  contentTarget.anchoredPosition.y) /
                                                                              maxElastic, 0, 1));
                        }
                        else if (contentTarget.anchoredPosition.y > contentTarget.sizeDelta.y - middle * 2)
                        {
                            inertia = 0;
                            contentTarget.anchoredPosition = new Vector2(0,
                                startPosContent + (-startPosMouse + (Input.mousePosition.y / camera.pixelHeight)
                                    * targetCanvas.sizeDelta.y) * Mathf.Clamp(1 - Mathf.Abs(
                                        (contentTarget.sizeDelta.y - middle * 2) -
                                        contentTarget.anchoredPosition.y) /
                                    maxElastic, 0, 1));
                        }
                        else
                        {
                            inertia = startPosContent + (-startPosMouse +
                                                         (Input.mousePosition.y / camera.pixelHeight) *
                                                         targetCanvas.sizeDelta.y) -
                                      contentTarget.anchoredPosition.y;
                            contentTarget.anchoredPosition = new Vector2(0,
                                startPosContent + (-startPosMouse + (Input.mousePosition.y /
                                                                     camera.pixelHeight) * targetCanvas.sizeDelta.y));
                        }

                        startPosMouse = (Input.mousePosition.y / camera.pixelHeight) * targetCanvas.sizeDelta.y;
                        startPosContent = contentTarget.anchoredPosition.y;
                    }
                    else
                    {
                        inertia = startPosContent + (-startPosMouse +
                                                     (Input.mousePosition.y / camera.pixelHeight) *
                                                     targetCanvas.sizeDelta.y) -
                                  contentTarget.anchoredPosition.y;
                        contentTarget.anchoredPosition = new Vector2(0, Mathf.Clamp(
                            startPosContent + (-startPosMouse + (Input.mousePosition.y /
                                                                 camera.pixelHeight) * targetCanvas.sizeDelta.y), 0,
                            contentTarget.sizeDelta.y - middle * 2));
                    }
                }
            }

            if (isInfinite)
            {
                if (contentTarget.anchoredPosition.y < middle)
                {
                    contentTarget.anchoredPosition = new Vector2(0, contentTarget.anchoredPosition.y +
                                                                    (padCount + (countTotal - padCount)) *
                                                                    (heightText * 2));
                    for (int i = 0; i < (padCount + (countTotal - padCount)); i++)
                    {
                        contentTarget.GetChild(i).GetChild(0).GetComponent<TextMeshProUGUI>().fontSize = 0;
                    }

                    startPosMouse = (Input.mousePosition.y / camera.pixelHeight) * targetCanvas.sizeDelta.y;
                    startPosContent = contentTarget.anchoredPosition.y;
                }
                else if (contentTarget.anchoredPosition.y > contentTarget.sizeDelta.y - middle * 3)
                {
                    contentTarget.anchoredPosition = new Vector2(0, contentTarget.anchoredPosition.y -
                                                                    (padCount + (countTotal - padCount)) *
                                                                    (heightText * 2));
                    for (int i = contentTarget.childCount - 1;
                         i >= contentTarget.childCount -
                         (padCount + (countTotal - padCount));
                         i--)
                    {
                        contentTarget.GetChild(i).GetChild(0).GetComponent<TextMeshProUGUI>().fontSize = 0;
                    }

                    startPosMouse = (Input.mousePosition.y / camera.pixelHeight) * targetCanvas.sizeDelta.y;
                    startPosContent = contentTarget.anchoredPosition.y;
                }
            }

            float contentPos = contentTarget.anchoredPosition.y;

            int startPoint = Mathf.CeilToInt((contentPos - (middle + heightText)) / (heightText * 2));
            int minID = Mathf.Max(0, startPoint);
            int maxID = Mathf.Min(contentTarget.transform.childCount, startPoint + countCheck + 1);
            minID = Mathf.Clamp(minID, 0, int.MaxValue);
            maxID = Mathf.Clamp(maxID, 0, int.MaxValue);
            /*currentCenter = Mathf.Clamp(Mathf.RoundToInt((contentPos - (middle + heightText)) / (heightText * 2)) +
                padCount, 0, contentTarget.childCount - 1);*/

            currentCenter = Mathf.Clamp(Mathf.RoundToInt(contentPos / (heightText * 2)), 0,
                contentTarget.childCount - 1);

            if (maxID > minID)
            {
                for (int i = minID; i < maxID; i++)
                {
                    var currentRect = contentTarget.transform.GetChild(i).GetComponent<RectTransform>();
                    var currentText = contentTarget.transform.GetChild(i).GetChild(0).GetComponent<TextMeshProUGUI>();
                    float ratio =
                        Mathf.Clamp(
                            1 - Mathf.Abs(contentPos + currentRect.anchoredPosition.y + middle) / (middle - padding), 0,
                            1);
                    if (contentPos + currentRect.anchoredPosition.y + middle > 0)
                    {
                        currentText.GetComponent<RectTransform>().anchoredPosition =
                            new Vector2(0, -curveShift.Evaluate(1 - ratio) * shiftUp);
                    }
                    else
                    {
                        currentText.GetComponent<RectTransform>().anchoredPosition =
                            new Vector2(0, curveShift.Evaluate(1 - ratio) * shiftDown);
                    }

                    currentText.fontSize = maxFontSize * curve.Evaluate(ratio);
                    currentText.color = new Vector4(currentText.color.r, currentText.color.g, currentText.color.b,
                        Mathf.Clamp((ratio - colorPad) / (1 - colorPad), 0, 1));
                }
            }

            if (Mathf.Abs(inertia) < minVelocity && !Input.GetMouseButton(0))
            {
                inertia = 0;
                contentTarget.anchoredPosition = new Vector2(0,
                    Mathf.Lerp(contentTarget.anchoredPosition.y,
                        -contentTarget.transform.GetChild(currentCenter).GetComponent<RectTransform>().anchoredPosition
                            .y - middle, speedLerp * Time.deltaTime));

                OnValueChanged(currentCenter);
            }
        }
    }

    /// <summary>
    /// 拖拽结束事件处理
    /// </summary>
    /// <param name="eventData">指针事件数据</param>
    public void OnDrop(PointerEventData eventData)
    {
        isDragging = false;
    }

    /// <summary>
    /// 拖拽过程事件处理（空实现，实际拖拽逻辑在Update中处理）
    /// </summary>
    /// <param name="eventData">指针事件数据</param>
    public void OnDrag(PointerEventData eventData)
    {
    }

    /// <summary>
    /// 拖拽开始事件处理，记录初始位置
    /// </summary>
    /// <param name="eventData">指针事件数据</param>
    public void OnBeginDrag(PointerEventData eventData)
    {
        isDragging = true;
        startPosMouse = (Input.mousePosition.y / camera.pixelHeight) * targetCanvas.sizeDelta.y;
        startPosContent = contentTarget.anchoredPosition.y;
    }

    /// <summary>鼠标是否在滚动区域内</summary>
    bool isInArea;

    /// <summary>
    /// 鼠标进入滚动区域事件处理
    /// </summary>
    /// <param name="eventData">指针事件数据</param>
    public void OnPointerEnter(PointerEventData eventData)
    {
        isInArea = true;
    }

    /// <summary>
    /// 鼠标离开滚动区域事件处理
    /// </summary>
    /// <param name="eventData">指针事件数据</param>
    public void OnPointerExit(PointerEventData eventData)
    {
        isInArea = false;
    }

    /// <summary>
    /// 当前选中值改变时的回调，用于设置播放按钮的时间间隔
    /// </summary>
    /// <param name="id">选中项的索引</param>
    public void OnValueChanged(int id)
    {
        float[] deltaTimes = new float[testData.Length];
        deltaTimes[0] = 0.1f;
        deltaTimes[1] = 0.2f;
        deltaTimes[2] = 0.5f;
        deltaTimes[3] = 1.0f;
        deltaTimes[4] = 2.0f;
        deltaTimes[5] = 5.0f;
        deltaTime.GetComponent<PlayButtonBehavior>().deltaTime = deltaTimes[id];
    }
}