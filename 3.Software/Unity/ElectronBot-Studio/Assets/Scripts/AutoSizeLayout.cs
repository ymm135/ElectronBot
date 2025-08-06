using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

/// <summary>
/// 自动尺寸布局组件
/// 提供自动排列子对象并调整容器大小的功能，支持垂直和水平布局
/// 可在编辑器模式下运行，支持递归更新和重复更新
/// </summary>
[ExecuteInEditMode]
public class AutoSizeLayout : MonoBehaviour
{
    /// <summary>是否在Update方法中循环更新布局</summary>
    public bool isLoopUpdate;
    /// <summary>是否为垂直布局</summary>
    public bool isVertical = true;
    /// <summary>是否需要调整自身大小</summary>
    public bool isResizeSelf = true;
    /// <summary>顶部内边距</summary>
    public float topPad;
    /// <summary>底部内边距</summary>
    public float bottomPad;
    /// <summary>左侧内边距</summary>
    public float leftPad;
    /// <summary>右侧内边距</summary>
    public float rightPad;
    /// <summary>子对象之间的间距</summary>
    public float spacing;
    /// <summary>首次更新后重复更新的帧数</summary>
    public int repeatFrames = 2;

    /// <summary>
    /// 每帧更新方法，根据isLoopUpdate标志决定是否持续更新布局
    /// </summary>
    private void Update() {
        if (isLoopUpdate) {
            UpdateLayout(false);
        }
    }

    /// <summary>
    /// 更新布局的主要方法
    /// </summary>
    /// <param name="isRepeat">是否重复更新多帧</param>
    /// <param name="isRecursive">是否递归更新子布局</param>
    public void UpdateLayout(bool isRepeat = true, bool isRecursive = false) {
        UpdateAllRect(isRecursive);
        if (isRepeat) {
            if(unpateRoutine != null) {
                StopCoroutine(unpateRoutine);
            }
            if (gameObject.activeInHierarchy) {
                unpateRoutine = StartCoroutine(UpdateRepeate(isRecursive));
            }
        }
    }

    /// <summary>
    /// 更新所有子对象的矩形变换，实现布局排列
    /// </summary>
    /// <param name="isRecursive">是否递归更新子布局组件</param>
    void UpdateAllRect(bool isRecursive) {
        if (isVertical) {
            // 垂直布局逻辑
            float sizeTotal = topPad;
            for (int i = 0; i < transform.childCount; i++) {
                if (transform.GetChild(i).tag != "NotInLayout" && transform.GetChild(i).gameObject.activeSelf) {
                    var rect = transform.GetChild(i).GetComponent<RectTransform>();
                    // 递归更新子布局
                    if (isRecursive) {
                        if (rect.GetComponent<AutoSizeLayout>()) {
                            rect.GetComponent<AutoSizeLayout>().UpdateLayout(isRecursive: true);
                        }
                    }
                    // 设置子对象位置
                    rect.anchoredPosition = new Vector2(leftPad - rightPad, -rect.sizeDelta.y * (1 - rect.pivot.y) - sizeTotal);
                    sizeTotal += rect.sizeDelta.y + spacing;
                }
            }
            sizeTotal -= spacing;
            sizeTotal += bottomPad;
            // 调整自身高度
            if (isResizeSelf) {
                GetComponent<RectTransform>().sizeDelta = new Vector2(GetComponent<RectTransform>().sizeDelta.x, sizeTotal);
            }
        } else {
            // 水平布局逻辑
            float sizeTotal = leftPad;
            for (int i = 0; i < transform.childCount; i++) {
                if (transform.GetChild(i).tag != "NotInLayout" && transform.GetChild(i).gameObject.activeSelf) {
                    var rect = transform.GetChild(i).GetComponent<RectTransform>();
                    // 递归更新子布局
                    if (isRecursive) {
                        if (rect.GetComponent<AutoSizeLayout>()) {
                            rect.GetComponent<AutoSizeLayout>().UpdateLayout(isRecursive: true);
                        }
                    }
                    // 设置子对象位置
                    rect.anchoredPosition = new Vector2(rect.sizeDelta.x * (1 - rect.pivot.x) + sizeTotal, topPad - bottomPad);
                    sizeTotal += rect.sizeDelta.x + spacing;
                }
            }
            sizeTotal -= spacing;
            sizeTotal += rightPad;
            // 调整自身宽度
            if (isResizeSelf) {
                GetComponent<RectTransform>().sizeDelta = new Vector2(sizeTotal, GetComponent<RectTransform>().sizeDelta.y);
            }
        }
    }

    /// <summary>重复更新的协程引用</summary>
    Coroutine unpateRoutine;
    
    /// <summary>
    /// 重复更新布局的协程，在指定帧数内持续更新
    /// </summary>
    /// <param name="isRecursive">是否递归更新</param>
    IEnumerator UpdateRepeate(bool isRecursive) {
        for(int i = 0; i < repeatFrames; i++) {
            yield return new WaitForEndOfFrame();
            UpdateAllRect(isRecursive);
        }
    }
}
