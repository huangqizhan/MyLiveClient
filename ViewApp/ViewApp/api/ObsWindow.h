#pragma once
#include <memory>
#include <vector>
#include <obs.hpp>
#include <graphics/vec2.h>
#include <graphics/matrix4.h>
#include "ObsWindowBase.h"


/*
 每一个画板对应一个display
 每个display会有自己的绘制回调函数
 每个display在obs_core_data->first_display上
*/

class ObsWindow :public ObsWindowBase
{
public:
    ObsWindow();
    ~ObsWindow();

    static ObsWindow*  Create();
    bool CreateDisplay();//创建obsdisplay

    obs_display_t* display() { return m_display; }

    void SetEnable(bool enable) { obs_display_set_enabled(m_display,enable); }
    bool IsEnable() { return obs_display_enabled(m_display); }
    ///四个顶点缓冲对象 
    gs_vertbuffer_t *box = nullptr;
    gs_vertbuffer_t *boxLeft = nullptr;
    gs_vertbuffer_t *boxTop = nullptr;
    gs_vertbuffer_t *boxRight = nullptr;
    gs_vertbuffer_t *boxBottom = nullptr;

//    void ResetVideo();
    
    void OnMousePressEvent(ObsMouseEvent *event) override;
    void OnMouseReleaseEvent(ObsMouseEvent *event) override;
    void OnMouseMoveEvent(ObsMouseEvent *event) override;
    void OnFocusChange(bool bGet) override;
    void OnDropFile(const char* file) override;
    void GetStretchHandleData(const vec2 &pos);
    
protected:
    static void _RenderWindow(void* param,uint32_t cx, uint32_t cy);
    void RenderWindow(uint32_t cx, uint32_t cy);
    void OnResize(const ObsSize& size) override;



    void InitPrimitives();
    void DrawBackdrop(float cx, float cy);
    void DrawSceneEditing();

    //是否锁定
    bool m_locked = false;
    OBSDisplay m_display;
    
    ///实际画布的大小跟video_info.base_width base_height的实际比例
    float m_previewScale = 1.0;
    
    ///画布上有效区域的宽高
    int m_previewCX = 0;
    int m_previewCY = 0;
    
    ///画布上有效区域的左上角
    int m_previewX = 0;
    int m_previewY = 0;
    
    vec2 GetMouseEventPos(ObsMouseEvent *event);
    static bool SelectedAtPos(const vec2 &pos);
    void DoSelect(const vec2 &pos);
    void DoCtrlSelect(const vec2 &pos);

    OBSSceneItem GetItemAtPos(const vec2 &pos, bool selectBelow);
    OBSSceneItem GetSelectItemAtPos(const vec2 &pos);

    void ProcessClick(const vec2 &pos) {
        DoSelect(pos);
    }

    vec3 CalculateStretchPos(const vec3 &tl, const vec3 &br);
    void ClampAspect(vec3 &tl, vec3 &br, vec2 &size,
        const vec2 &baseSize);

    vec3 GetSnapOffset(const vec3 &tl, const vec3 &br);
    void SnapItemMovement(vec2 &offset);
    void MoveItems(const vec2 &pos);
    void CropItem(const vec2 &pos);
    void StretchItem(const vec2 &pos);

    //鼠标事件
    obs_sceneitem_crop startCrop;
    vec2         startItemPos;
    vec2         cropSize;
    OBSSceneItem stretchGroup;
    ///当前选中 需要变换操作的item
    OBSSceneItem stretchItem;
    ///画布上点击时  是否有控制点响应
    ItemHandle   stretchHandle = ItemHandle::None;
    ///当前item放缩后的实际大小
    vec2         stretchItemSize;
    matrix4      screenToItem;
    matrix4      itemToScreen;
    matrix4      invGroupTransform;

    ObsSize      size;
    ///鼠标第一次按下时的位置 在base_width  base_height的空间下 
    vec2         startPos;
    ///拖动鼠标时的上一次距开始位置的偏移量
    vec2         lastMoveOffset;
    bool         cropping = false;
    bool         mouseDown = false;
    bool         rmouseDown = false;
    bool         mouseMoved = false;
    ///按下鼠标是是否有item被选中
    bool         mouseOverItems = false;
};



