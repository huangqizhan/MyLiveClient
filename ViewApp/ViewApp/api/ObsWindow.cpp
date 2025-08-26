#include "ObsWindow.h"
#include "ObsMain.h"
#include "ObsPlatform.h"
#include <graphics/vec4.h>
#include <graphics/matrix4.h>
#include <algorithm>
#include <functional>

#define HANDLE_RADIUS     4.0f
#define HANDLE_SEL_RADIUS (HANDLE_RADIUS * 1.5f)
#define PREVIEW_EDGE_SIZE 4

bool ObsWindow::CreateDisplay()
{
    if (m_display)
        return false;
    window_handle_t handle = GetWndHandle();
    if (!handle)
    {
        blog(LOG_WARNING, "window not created cant create display");
        return false;
    }

    ObsSize size = GetClientSize();

    gs_init_data info = {};
    info.cx = size.width;
    info.cy = size.height;
    info.format = GS_RGBA;
    info.zsformat = GS_ZS_NONE;
    info.window.view = (id)handle;

    m_display = obs_display_create(&info, 0x161616);
    obs_display_add_draw_callback(m_display, _RenderWindow, this);

    InitPrimitives();

    OnResize(size);

    return true;
}

ObsWindow::ObsWindow()
{
    matrix4_identity(&screenToItem);
    matrix4_identity(&itemToScreen);
    matrix4_identity(&invGroupTransform);
    vec2_zero(&startItemPos);
    vec2_zero(&cropSize);
    vec2_zero(&stretchItemSize);
    vec2_zero(&startPos);
    vec2_zero(&lastMoveOffset);
}

ObsWindow::~ObsWindow()
{
    obs_display_remove_draw_callback(m_display,
        ObsWindow::_RenderWindow, this);

    obs_enter_graphics();
    gs_vertexbuffer_destroy(box);
    gs_vertexbuffer_destroy(boxLeft);
    gs_vertexbuffer_destroy(boxTop);
    gs_vertexbuffer_destroy(boxRight);
    gs_vertexbuffer_destroy(boxBottom);
    obs_leave_graphics();
}


void ObsWindow::InitPrimitives()
{
    obs_enter_graphics();

    gs_render_start(true);
    gs_vertex2f(0.0f, 0.0f);//左下
    gs_vertex2f(0.0f, 1.0f);//左上
    gs_vertex2f(1.0f, 1.0f);//右下
    gs_vertex2f(1.0f, 0.0f);//右上
    gs_vertex2f(0.0f, 0.0f);
    box = gs_render_save();

    gs_render_start(true);
    gs_vertex2f(0.0f, 0.0f);
    gs_vertex2f(0.0f, 1.0f);
    boxLeft = gs_render_save();

    gs_render_start(true);
    gs_vertex2f(0.0f, 0.0f);
    gs_vertex2f(1.0f, 0.0f);
    boxTop = gs_render_save();

    gs_render_start(true);
    gs_vertex2f(1.0f, 0.0f);
    gs_vertex2f(1.0f, 1.0f);
    boxRight = gs_render_save();

    gs_render_start(true);
    gs_vertex2f(0.0f, 1.0f);
    gs_vertex2f(1.0f, 1.0f);
    boxBottom = gs_render_save();


    obs_leave_graphics();
}
///用solid.effect 绘制背景色
void ObsWindow::DrawBackdrop(float cx, float cy)
{
    if (!box)
        return;

    gs_effect_t    *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
    gs_eparam_t    *color = gs_effect_get_param_by_name(solid, "color");
    gs_technique_t *tech = gs_effect_get_technique(solid, "Solid");

    //给着色器参数设值
    vec4 colorVal;
    vec4_set(&colorVal, 0.4f, 0.4f, 0.4f, 1.0f);
    gs_effect_set_vec4(color, &colorVal);
    
    
    gs_technique_begin(tech);
    gs_technique_begin_pass(tech, 0);
    gs_matrix_push();
    
    gs_matrix_identity();
    gs_matrix_scale3f(float(cx), float(cy), 1.0f);

    gs_load_vertexbuffer(box);
    gs_draw(GS_TRISTRIP, 0, 0);

    
    gs_matrix_pop();
    gs_technique_end_pass(tech);
    gs_technique_end(tech);

    gs_load_vertexbuffer(nullptr);
}
///绘制边框上的各个顶点
static void DrawSquareAtPos(float x, float y)
{
    struct vec3 pos;
    vec3_set(&pos, x, y, 0.0f);

    struct matrix4 matrix;
    ///此处取上一次 gs_matrix_push() 的矩阵 也就是boxMatrix
    gs_matrix_get(&matrix);
    ///把pos变换到box的变换层面
    vec3_transform(&pos, &pos, &matrix);
    
    ///再上一个新的矩阵 右是一层新的变换   也就是点本身变换层面
    gs_matrix_push();
    ///单位矩阵
    gs_matrix_identity();
    ///移动到顶点位置
    gs_matrix_translate(&pos);
    ///把顶点的中心移动到边线的中心
    gs_matrix_translate3f(-HANDLE_RADIUS, -HANDLE_RADIUS, 0.0f);
    ///放大（初始值是[0,1]）(InitPrimitives ==>box)
    gs_matrix_scale3f(HANDLE_RADIUS * 2, HANDLE_RADIUS * 2, 1.0f);
    ///绘制正方形顶点
    gs_draw(GS_TRISTRIP, 0, 0);
    gs_matrix_pop();
}

static inline bool crop_enabled(const obs_sceneitem_crop *crop)
{
    return crop->left > 0 ||
        crop->top > 0 ||
        crop->right > 0 ||
        crop->bottom > 0;
}

static bool SceneItemHasVideo(obs_sceneitem_t *item)
{
    obs_source_t *source = obs_sceneitem_get_source(item);
    uint32_t flags = obs_source_get_output_flags(source);
    return (flags & OBS_SOURCE_VIDEO) != 0;
}

static bool CloseFloat(float a, float b, float epsilon = 0.01)
{
    using std::abs;
    return abs(a - b) <= epsilon;
}
///绘制item的边框
bool DrawSelectedItem(obs_scene_t *scene, obs_sceneitem_t *item, void *param) {
    if (obs_sceneitem_locked(item))
        return true;

    if (!SceneItemHasVideo(item))
        return true;

    if (obs_sceneitem_is_group(item)) {
        matrix4 mat;
        obs_sceneitem_get_draw_transform(item, &mat);

        gs_matrix_push();
        gs_matrix_mul(&mat);
        obs_sceneitem_group_enum_items(item, DrawSelectedItem, param);
        gs_matrix_pop();
    }

    if (!obs_sceneitem_selected(item))
        return true;
    ///scene_item的变换矩阵
    matrix4 boxTransform;
    ///scene_item的变换矩阵的逆矩阵  可以变换到box本身坐标系 (如果没有放大的话 则是[0,1])
    matrix4 invBoxTransform;
    obs_sceneitem_get_box_transform(item, &boxTransform);
    matrix4_inv(&invBoxTransform, &boxTransform);
    ///顶点数组
    vec3 bounds[] = {
        { { { 0.f, 0.f, 0.f } } },
        { { { 1.f, 0.f, 0.f } } },
        { { { 0.f, 1.f, 0.f } } },
        { { { 1.f, 1.f, 0.f } } },
    };
    ///是否出界
    bool visible = std::all_of(std::begin(bounds), std::end(bounds),
        [&](const vec3 &b)
    {
        vec3 pos;
        vec3_transform(&pos, &b, &boxTransform);        ///先变换
        vec3_transform(&pos, &pos, &invBoxTransform);   ///再逆变换
        return CloseFloat(pos.x, b.x) && CloseFloat(pos.y, b.y);
    });

    if (!visible)
        return true;

    obs_transform_info info;
    obs_sceneitem_get_info(item, &info);

    ObsWindow* window = (ObsWindow*)param;
    gs_load_vertexbuffer(window->box);
    ///上一个矩阵 变换到box的坐标系
    gs_matrix_push();
    gs_matrix_mul(&boxTransform);

    ///画八个小正方形
    DrawSquareAtPos(0.0f, 0.0f);
    DrawSquareAtPos(0.0f, 1.0f);
    DrawSquareAtPos(1.0f, 0.0f);
    DrawSquareAtPos(1.0f, 1.0f);
    DrawSquareAtPos(0.5f, 0.0f);
    DrawSquareAtPos(0.0f, 0.5f);
    DrawSquareAtPos(0.5f, 1.0f);
    DrawSquareAtPos(1.0f, 0.5f);

    obs_sceneitem_crop crop;
    obs_sceneitem_get_crop(item, &crop);

    if (info.bounds_type == OBS_BOUNDS_NONE && crop_enabled(&crop)) {
        vec4 color;
        gs_effect_t *eff = gs_get_effect();
        gs_eparam_t *param = gs_effect_get_param_by_name(eff, "color");

#define DRAW_SIDE(side, vb) \
        if (crop.side > 0) \
            vec4_set(&color, 0.0f, 1.0f, 0.0f, 1.0f); \
        else \
            vec4_set(&color, 1.0f, 0.0f, 0.0f, 1.0f); \
        gs_effect_set_vec4(param, &color); \
        gs_load_vertexbuffer(window->vb); \
        gs_draw(GS_LINESTRIP, 0, 0);

        DRAW_SIDE(left, boxLeft);
        DRAW_SIDE(top, boxTop);
        DRAW_SIDE(right, boxRight);
        DRAW_SIDE(bottom, boxBottom);
#undef DRAW_SIDE
    }
    else {
        gs_load_vertexbuffer(window->box);
        gs_draw(GS_LINESTRIP, 0, 0);
    }

    gs_matrix_pop();

    UNUSED_PARAMETER(scene);
    UNUSED_PARAMETER(param);
    return true;
}
///=== 绘制当前的scene下选中的所有的item的边框
void ObsWindow::DrawSceneEditing()
{
    if (m_locked)
        return;
    ///边框effect
    gs_effect_t    *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
    gs_technique_t *tech = gs_effect_get_technique(solid, "Solid");
    
    vec4 color;
    //设置边框颜色
    vec4_set(&color, 1.0f, 0.0f, 0.0f, 1.0f);
    gs_effect_set_vec4(gs_effect_get_param_by_name(solid, "color"), &color);
    
    gs_technique_begin(tech);
    gs_technique_begin_pass(tech, 0);

    OBSScene scene = ObsMain::Instance()->GetCurrentScene();
    if (scene) {
        gs_matrix_push();
        gs_matrix_scale3f(m_previewScale, m_previewScale, 1.0f);
        obs_scene_enum_items(scene, DrawSelectedItem, this);
        gs_matrix_pop();
    }

    gs_load_vertexbuffer(nullptr);

    gs_technique_end_pass(tech);
    gs_technique_end(tech);
}


void ObsWindow::_RenderWindow(void* param, uint32_t cx, uint32_t cy)
{
    ObsWindow* window = (ObsWindow*)param;
    window->RenderWindow(cx, cy);
}
///===绘制当前scene的帧缓冲区帧到屏幕上 以及当前scene下选中的item边框 
void ObsWindow::RenderWindow(uint32_t cx, uint32_t cy)
{
    if (!GetWndHandle())
        return;

    obs_video_info ovi;

    obs_get_video_info(&ovi);

    m_previewCX = int(m_previewScale * float(ovi.base_width));
    m_previewCY = int(m_previewScale * float(ovi.base_height));

    ///给viewport栈 添加一个新的viewport
    gs_viewport_push();
    ///给proj_stack栈 添加一个新的透视矩阵
    gs_projection_push();

    ///给新的透视矩阵设值
    gs_ortho(0.0f, float(ovi.base_width), 0.0f, float(ovi.base_height), -100.0f, 100.0f);
    ///给添加的新的viewport设值
    gs_set_viewport(m_previewX, m_previewY, m_previewCX, m_previewCY);
    ///绘制背景
    DrawBackdrop(float(ovi.base_width), float(ovi.base_height));
    //绘制缓冲区中的纹理到屏幕上
    obs_render_main_texture();
    ///置空device的顶点缓冲区
    gs_load_vertexbuffer(nullptr);
    
    ///
    ObsSize size = GetClientSize();
    
    float right = float(size.width) - m_previewX;
    float bottom = float(size.height) - m_previewY;
    gs_ortho(-m_previewX, right, -m_previewY, bottom, -100.0f, 100.0f);
    gs_reset_viewport();
    ///绘制scene边框
    DrawSceneEditing();
    ///退出新的透视矩阵
    gs_projection_pop();
    ///退出新的viewport
    gs_viewport_pop();
}
///====根据video_info base_width(1280) base_height(960)和实际画布的大小 计算出放缩比和起始位置
static inline void GetScaleAndCenterPos(
    int baseCX, int baseCY, int windowCX, int windowCY,
    int &x, int &y, float &scale)
{
    double windowAspect, baseAspect;
    int newCX, newCY;

    windowAspect = double(windowCX) / double(windowCY);
    baseAspect = double(baseCX) / double(baseCY);

    if (windowAspect > baseAspect) {
        scale = float(windowCY) / float(baseCY);
        newCX = int(double(windowCY) * baseAspect);
        newCY = windowCY;
    }
    else {
        scale = float(windowCX) / float(baseCX);
        newCX = windowCX;
        newCY = int(float(windowCX) / baseAspect);
    }

    x = windowCX / 2 - newCX / 2;
    y = windowCY / 2 - newCY / 2;
}

//void ObsWindow::ResetVideo()
//{
//    obs_video_info ovi;
//    if (obs_get_video_info(&ovi))
//    {
//        GetScaleAndCenterPos(int(ovi.base_width), int(ovi.base_height),
//            size.width - PREVIEW_EDGE_SIZE * 2,
//            size.height - PREVIEW_EDGE_SIZE * 2,
//            m_previewX, m_previewY, m_previewScale);
//
//
//        m_previewX += float(PREVIEW_EDGE_SIZE);
//        m_previewY += float(PREVIEW_EDGE_SIZE);
//    }
//}

void ObsWindow::OnResize(const ObsSize& size)
{
    if (m_display)
    {
        obs_display_resize(m_display, size.width, size.height);
    }

    this->size = size;

    obs_video_info ovi;
    if (obs_get_video_info(&ovi))
    {
        GetScaleAndCenterPos(int(ovi.base_width), int(ovi.base_height),
            size.width - PREVIEW_EDGE_SIZE * 2,
            size.height - PREVIEW_EDGE_SIZE * 2,
            m_previewX, m_previewY, m_previewScale);


        m_previewX += float(PREVIEW_EDGE_SIZE);
        m_previewY += float(PREVIEW_EDGE_SIZE);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
//鼠标事件处理
static vec3 GetTransformedPos(float x, float y, const matrix4 &mat)
{
    vec3 result;
    vec3_set(&result, x, y, 0.0f);
    vec3_transform(&result, &result, &mat);
    return result;
}


struct HandleFindData {
    ///在base_width  base_height上  当前要查找的位置
    const vec2    &pos;
    ///以pos为圆心radius为半径 如果在半径内 则handle所对应的触点相应
    const float   radius;
    ///item对应的变换矩阵
    matrix4       parent_xform;
    ///当前pos位置查找到的item
    OBSSceneItem item;
    ItemHandle   handle = ItemHandle::None;

    HandleFindData(const HandleFindData &) = delete;
    HandleFindData(HandleFindData &&) = delete;
    HandleFindData& operator=(const HandleFindData &) = delete;
    HandleFindData& operator=(HandleFindData &&) = delete;

    inline HandleFindData(const vec2 &pos_, float scale)
        : pos(pos_),
        radius(HANDLE_SEL_RADIUS / scale)
    {
        matrix4_identity(&parent_xform);
    }

    inline HandleFindData(const HandleFindData &hfd,
        obs_sceneitem_t *parent)
        : pos(hfd.pos),
        radius(hfd.radius),
        item(hfd.item),
        handle(hfd.handle)
    {
        obs_sceneitem_get_draw_transform(parent, &parent_xform);
    }
};
///======查找当前位置附近是否有是否有控制点
static bool FindHandleAtPos(obs_scene_t *scene, obs_sceneitem_t *item,
    void *param)
{
    HandleFindData &data = *reinterpret_cast<HandleFindData*>(param);

    if (!obs_sceneitem_selected(item)) {
        if (obs_sceneitem_is_group(item)) {
            HandleFindData newData(data, item);
            obs_sceneitem_group_enum_items(item, FindHandleAtPos,
                &newData);
            data.item = newData.item;
            data.handle = newData.handle;
        }

        return true;
    }

    matrix4        transform;
    vec3           pos3;
    float          closestHandle = data.radius;

    vec3_set(&pos3, data.pos.x, data.pos.y, 0.0f);
    ///变换到item->box_transform层面
    obs_sceneitem_get_box_transform(item, &transform);
    
    ///遍历每一个控制点 是否在pos3附近
    auto TestHandle = [&](float x, float y, ItemHandle handle){
        ///计算(x, y)在box_transform层面的位置
        vec3 handlePos = GetTransformedPos(x, y, transform);
//        vec3_transform(&handlePos, &handlePos, &data.parent_xform);
        float dist = vec3_dist(&handlePos, &pos3);
        ///在data.radius半径内当前的handle开始响应
        if (dist < data.radius) {
            if (dist < closestHandle) {
                closestHandle = dist;
                data.handle = handle;
                data.item = item;
            }
        }
    };
    ///查找合适的控制点
    TestHandle(0.0f, 0.0f, ItemHandle::TopLeft);
    TestHandle(0.5f, 0.0f, ItemHandle::TopCenter);
    TestHandle(1.0f, 0.0f, ItemHandle::TopRight);
    TestHandle(0.0f, 0.5f, ItemHandle::CenterLeft);
    TestHandle(1.0f, 0.5f, ItemHandle::CenterRight);
    TestHandle(0.0f, 1.0f, ItemHandle::BottomLeft);
    TestHandle(0.5f, 1.0f, ItemHandle::BottomCenter);
    TestHandle(1.0f, 1.0f, ItemHandle::BottomRight);

    UNUSED_PARAMETER(scene);
    return true;
}
///当前item的实际大小
static vec2 GetItemSize(obs_sceneitem_t *item)
{
    obs_bounds_type boundsType = obs_sceneitem_get_bounds_type(item);
    vec2 size;

    if (boundsType != OBS_BOUNDS_NONE) {
        obs_sceneitem_get_bounds(item, &size);
    }
    else {
        ///item的实际大小
        obs_source_t *source = obs_sceneitem_get_source(item);
        obs_sceneitem_crop crop;
        vec2 scale;

        obs_sceneitem_get_scale(item, &scale);
        obs_sceneitem_get_crop(item, &crop);
        size.x = float(obs_source_get_width(source) -
            crop.left - crop.right) * scale.x;
        size.y = float(obs_source_get_height(source) -
            crop.top - crop.bottom) * scale.y;
    }

    return size;
}
///====查找控制点
void ObsWindow::GetStretchHandleData(const vec2 &pos)
{

    OBSScene scene = ObsMain::Instance()->GetCurrentScene();
    if (!scene)
        return;

    float scale = m_previewScale;
    vec2 scaled_pos = pos;
    ///把当前pos的位置转换成video_info.base_width base_height的坐标
    vec2_divf(&scaled_pos, &scaled_pos, scale);
    HandleFindData data(scaled_pos, scale);
    obs_scene_enum_items(scene, FindHandleAtPos, &data);

    stretchItem = std::move(data.item);
    stretchHandle = data.handle;

    ///有控制点响应 
    if (stretchHandle != ItemHandle::None) {
        matrix4 boxTransform;
        vec3    itemUL;
        float   itemRot;

        stretchItemSize = GetItemSize(stretchItem);

        obs_sceneitem_get_box_transform(stretchItem, &boxTransform);
        itemRot = obs_sceneitem_get_rot(stretchItem);
        ///最后一列用作平移
        vec3_from_vec4(&itemUL, &boxTransform.t);

        /* build the item space conversion matrices */
        matrix4_identity(&itemToScreen);
        matrix4_rotate_aa4f(&itemToScreen, &itemToScreen, 0.0f, 0.0f, 1.0f, RAD(itemRot));
        matrix4_translate3f(&itemToScreen, &itemToScreen, itemUL.x, itemUL.y, 0.0f);

        matrix4_identity(&screenToItem);
        matrix4_translate3f(&screenToItem, &screenToItem, -itemUL.x, -itemUL.y, 0.0f);
        matrix4_rotate_aa4f(&screenToItem, &screenToItem, 0.0f, 0.0f, 1.0f, RAD(-itemRot));

        obs_sceneitem_get_crop(stretchItem, &startCrop);
        obs_sceneitem_get_pos(stretchItem, &startItemPos);

        obs_source_t *source = obs_sceneitem_get_source(stretchItem);
        cropSize.x = float(obs_source_get_width(source) - startCrop.left - startCrop.right);
        cropSize.y = float(obs_source_get_height(source) - startCrop.top - startCrop.bottom);

        stretchGroup = obs_sceneitem_get_group(scene, stretchItem);
        if (stretchGroup) {
            obs_sceneitem_get_draw_transform(stretchGroup, &invGroupTransform);
            matrix4_inv(&invGroupTransform, &invGroupTransform);
            obs_sceneitem_defer_group_resize_begin(stretchGroup);
        }
    }
}

struct SceneFindData {
    const vec2   &pos;
    OBSSceneItem item;
    bool         selectBelow;

    obs_sceneitem_t *group = nullptr;

    SceneFindData(const SceneFindData &) = delete;
    SceneFindData(SceneFindData &&) = delete;
    SceneFindData& operator=(const SceneFindData &) = delete;
    SceneFindData& operator=(SceneFindData &&) = delete;

    inline SceneFindData(const vec2 &pos_, bool selectBelow_)
        : pos(pos_),
        selectBelow(selectBelow_)
    {}
};
///=== 查找ceneitem return true 继续查找下一个 false表示已经找到
static bool CheckItemSelected(obs_scene_t *scene, obs_sceneitem_t *item,
    void *param)
{
    SceneFindData *data = reinterpret_cast<SceneFindData*>(param);
    matrix4       transform;
    vec3          transformedPos;
    vec3          pos3;

    if (!SceneItemHasVideo(item))
        return true;
    //scene_group 递归
    if (obs_sceneitem_is_group(item)) {
        data->group = item;
        obs_sceneitem_group_enum_items(item, CheckItemSelected, param);
        data->group = nullptr;

        if (data->item) {
            return false;
        }
    }

    vec3_set(&pos3, data->pos.x, data->pos.y, 0.0f);
    obs_sceneitem_get_box_transform(item, &transform);

    if (data->group) {
        matrix4 parent_transform;
        obs_sceneitem_get_draw_transform(data->group, &parent_transform);
        matrix4_mul(&transform, &transform, &parent_transform);
    }
    ///变换回item子空间
    matrix4_inv(&transform, &transform);
    vec3_transform(&transformedPos, &pos3, &transform);
    if (transformedPos.x >= 0.0f && transformedPos.x <= 1.0f &&
        transformedPos.y >= 0.0f && transformedPos.y <= 1.0f) {
        if (obs_sceneitem_selected(item)) {
            data->item = item;
            return false;
        }
    }
    UNUSED_PARAMETER(scene);
    return true;
}
///====触点是否有可选中的item
bool ObsWindow::SelectedAtPos(const vec2 &pos)
{
    OBSScene scene = ObsMain::Instance()->GetCurrentScene();
    if (!scene)
        return false;

    SceneFindData data(pos, false);
    obs_scene_enum_items(scene, CheckItemSelected, &data);
    return !!data.item;
}
///===按下鼠标
void ObsWindow::OnMousePressEvent(ObsMouseEvent *event)
{
    if (m_locked) {
        return;
    }
    //当前触点相对于左上角的移动量
    float x = float(event->x) - m_previewX;
    float y = float(event->y) - m_previewY;

    if (event->button != LeftButton &&
        event->button != RightButton)
        return;

    if (event->button == LeftButton)
        mouseDown = true;
    else
        rmouseDown = true;


    vec2_set(&startPos, x, y);
    ///查找控制点
    GetStretchHandleData(startPos);
    
    ///换算到 base_width  base_height的空间下
    vec2_divf(&startPos, &startPos, m_previewScale);
    startPos.x = std::round(startPos.x);
    startPos.y = std::round(startPos.y);

    mouseOverItems = SelectedAtPos(startPos);
    vec2_zero(&lastMoveOffset);
}
///把画布的坐标转换到 base_height  base_width上
vec2 ObsWindow::GetMouseEventPos(ObsMouseEvent *event)
{
    float pixelRatio = 1.0;
    float scale = pixelRatio / m_previewScale;
    vec2 pos;
    vec2_set(&pos,
        (float(event->x) - m_previewX / pixelRatio) * scale,
        (float(event->y) - m_previewY / pixelRatio) * scale);

    return pos;
}
static bool FindItemAtPos(obs_scene_t *scene, obs_sceneitem_t *item,
    void *param)
{
    SceneFindData *data = reinterpret_cast<SceneFindData*>(param);
    matrix4       transform;
    matrix4       invTransform;
    vec3          transformedPos;
    vec3          pos3;
    vec3          pos3_;

    if (!SceneItemHasVideo(item))
        return true;
    if (obs_sceneitem_locked(item))
        return true;

    vec3_set(&pos3, data->pos.x, data->pos.y, 0.0f);

    obs_sceneitem_get_box_transform(item, &transform);
    ///求transform逆变换   变换到item的子空间
    matrix4_inv(&invTransform, &transform);
    ///pos3变换到 变换到item的子空间
    vec3_transform(&transformedPos, &pos3, &invTransform);
    ///再变换到父空间
    vec3_transform(&pos3_, &transformedPos, &transform);
    ///这样变换之后子空间和父空间的坐标 就都有了
    if (CloseFloat(pos3.x, pos3_.x) && CloseFloat(pos3.y, pos3_.y) &&
        transformedPos.x >= 0.0f && transformedPos.x <= 1.0f &&
        transformedPos.y >= 0.0f && transformedPos.y <= 1.0f) {
        if (data->selectBelow && obs_sceneitem_selected(item)) {
            if (data->item)
                return false;
            else
                data->selectBelow = false;
        }

        data->item = item;
    }

    UNUSED_PARAMETER(scene);
    return true;
}

OBSSceneItem ObsWindow::GetItemAtPos(const vec2 &pos, bool selectBelow)
{
    OBSScene scene = ObsMain::Instance()->GetCurrentScene();
    if (!scene)
        return OBSSceneItem();

    SceneFindData data(pos, selectBelow);
    obs_scene_enum_items(scene, FindItemAtPos, &data);
    return data.item;
}


static bool FindSelectItemAtPos(obs_scene_t *scene, obs_sceneitem_t *item,
    void *param)
{
    SceneFindData *data = reinterpret_cast<SceneFindData*>(param);
    matrix4       transform;
    matrix4       invTransform;
    vec3          transformedPos;
    vec3          pos3;
    vec3          pos3_;

    if (!SceneItemHasVideo(item))
        return true;
    if (obs_sceneitem_locked(item))
        return true;

    vec3_set(&pos3, data->pos.x, data->pos.y, 0.0f);

    obs_sceneitem_get_box_transform(item, &transform);

    matrix4_inv(&invTransform, &transform);
    vec3_transform(&transformedPos, &pos3, &invTransform);
    vec3_transform(&pos3_, &transformedPos, &transform);

    if (CloseFloat(pos3.x, pos3_.x) && CloseFloat(pos3.y, pos3_.y) &&
        transformedPos.x >= 0.0f && transformedPos.x <= 1.0f &&
        transformedPos.y >= 0.0f && transformedPos.y <= 1.0f) {
        if (obs_sceneitem_selected(item)) {
            data->item = item;
            return false;
        }
    }
    UNUSED_PARAMETER(scene);
    return true;
}
OBSSceneItem ObsWindow::GetSelectItemAtPos(const vec2 &pos)
{
    OBSScene scene = ObsMain::Instance()->GetCurrentScene();
    if (!scene)
        return OBSSceneItem();

    SceneFindData data(pos, false);
    obs_scene_enum_items(scene, FindSelectItemAtPos, &data);
    return data.item;
}


static bool select_one(obs_scene_t *scene, obs_sceneitem_t *item, void *param)
{
    obs_sceneitem_t *selectedItem =
        reinterpret_cast<obs_sceneitem_t*>(param);
    if (obs_sceneitem_is_group(item))
        obs_sceneitem_group_enum_items(item, select_one, param);

    obs_sceneitem_select(item, (selectedItem == item));

    UNUSED_PARAMETER(scene);
    return true;
}

void ObsWindow::DoSelect(const vec2 &pos)
{
    OBSScene     scene =ObsMain::Instance()->GetCurrentScene();
    OBSSceneItem item = GetItemAtPos(pos, true);

    obs_scene_enum_items(scene, select_one, (obs_sceneitem_t*)item);
}

void ObsWindow::DoCtrlSelect(const vec2 &pos)
{
    OBSSceneItem item = GetItemAtPos(pos, false);
    if (!item)
        return;

    bool selected = obs_sceneitem_selected(item);
    obs_sceneitem_select(item, !selected);
}

vec3 ObsWindow::CalculateStretchPos(const vec3 &tl, const vec3 &br)
{
    uint32_t alignment = obs_sceneitem_get_alignment(stretchItem);
    vec3 pos;

    vec3_zero(&pos);

    if (alignment & OBS_ALIGN_LEFT)
        pos.x = tl.x;
    else if (alignment & OBS_ALIGN_RIGHT)
        pos.x = br.x;
    else
        pos.x = (br.x - tl.x) * 0.5f + tl.x;

    if (alignment & OBS_ALIGN_TOP)
        pos.y = tl.y;
    else if (alignment & OBS_ALIGN_BOTTOM)
        pos.y = br.y;
    else
        pos.y = (br.y - tl.y) * 0.5f + tl.y;

    return pos;
}

void ObsWindow::ClampAspect(vec3 &tl, vec3 &br, vec2 &size,
    const vec2 &baseSize)
{
    float    baseAspect = baseSize.x / baseSize.y;
    float    aspect = size.x / size.y;
    uint32_t stretchFlags = (uint32_t)stretchHandle;

    if (stretchHandle == ItemHandle::TopLeft ||
        stretchHandle == ItemHandle::TopRight ||
        stretchHandle == ItemHandle::BottomLeft ||
        stretchHandle == ItemHandle::BottomRight) {
        if (aspect < baseAspect) {
            if ((size.y >= 0.0f && size.x >= 0.0f) ||
                (size.y <= 0.0f && size.x <= 0.0f))
                size.x = size.y * baseAspect;
            else
                size.x = size.y * baseAspect * -1.0f;
        }
        else {
            if ((size.y >= 0.0f && size.x >= 0.0f) ||
                (size.y <= 0.0f && size.x <= 0.0f))
                size.y = size.x / baseAspect;
            else
                size.y = size.x / baseAspect * -1.0f;
        }

    }
    else if (stretchHandle == ItemHandle::TopCenter ||
        stretchHandle == ItemHandle::BottomCenter) {
        if ((size.y >= 0.0f && size.x >= 0.0f) ||
            (size.y <= 0.0f && size.x <= 0.0f))
            size.x = size.y * baseAspect;
        else
            size.x = size.y * baseAspect * -1.0f;

    }
    else if (stretchHandle == ItemHandle::CenterLeft ||
        stretchHandle == ItemHandle::CenterRight) {
        if ((size.y >= 0.0f && size.x >= 0.0f) ||
            (size.y <= 0.0f && size.x <= 0.0f))
            size.y = size.x / baseAspect;
        else
            size.y = size.x / baseAspect * -1.0f;
    }

    size.x = std::round(size.x);
    size.y = std::round(size.y);

    if (stretchFlags & ITEM_LEFT)
        tl.x = br.x - size.x;
    else if (stretchFlags & ITEM_RIGHT)
        br.x = tl.x + size.x;

    if (stretchFlags & ITEM_TOP)
        tl.y = br.y - size.y;
    else if (stretchFlags & ITEM_BOTTOM)
        br.y = tl.y + size.y;
}


static float maxfunc(float x, float y)
{
    return x > y ? x : y;
}

static float minfunc(float x, float y)
{
    return x < y ? x : y;
}

void ObsWindow::CropItem(const vec2 &pos)
{
    obs_bounds_type boundsType = obs_sceneitem_get_bounds_type(stretchItem);
    uint32_t stretchFlags = (uint32_t)stretchHandle;
    uint32_t align = obs_sceneitem_get_alignment(stretchItem);
    vec3 tl, br, pos3;

    vec3_zero(&tl);
    vec3_set(&br, stretchItemSize.x, stretchItemSize.y, 0.0f);

    vec3_set(&pos3, pos.x, pos.y, 0.0f);
    vec3_transform(&pos3, &pos3, &screenToItem);

    obs_sceneitem_crop crop = startCrop;
    vec2 scale;

    obs_sceneitem_get_scale(stretchItem, &scale);

    vec2 max_tl;
    vec2 max_br;

    vec2_set(&max_tl,
        float(-crop.left) * scale.x,
        float(-crop.top) * scale.y);
    vec2_set(&max_br,
        stretchItemSize.x + crop.right * scale.x,
        stretchItemSize.y + crop.bottom * scale.y);

    typedef std::function<float(float, float)> minmax_func_t;

    minmax_func_t min_x = scale.x < 0.0f ? maxfunc : minfunc;
    minmax_func_t min_y = scale.y < 0.0f ? maxfunc : minfunc;
    minmax_func_t max_x = scale.x < 0.0f ? minfunc : maxfunc;
    minmax_func_t max_y = scale.y < 0.0f ? minfunc : maxfunc;

    pos3.x = min_x(pos3.x, max_br.x);
    pos3.x = max_x(pos3.x, max_tl.x);
    pos3.y = min_y(pos3.y, max_br.y);
    pos3.y = max_y(pos3.y, max_tl.y);

    if (stretchFlags & ITEM_LEFT) {
        float maxX = stretchItemSize.x - (2.0 * scale.x);
        pos3.x = tl.x = min_x(pos3.x, maxX);

    }
    else if (stretchFlags & ITEM_RIGHT) {
        float minX = (2.0 * scale.x);
        pos3.x = br.x = max_x(pos3.x, minX);
    }

    if (stretchFlags & ITEM_TOP) {
        float maxY = stretchItemSize.y - (2.0 * scale.y);
        pos3.y = tl.y = min_y(pos3.y, maxY);

    }
    else if (stretchFlags & ITEM_BOTTOM) {
        float minY = (2.0 * scale.y);
        pos3.y = br.y = max_y(pos3.y, minY);
    }

#define ALIGN_X (ITEM_LEFT|ITEM_RIGHT)
#define ALIGN_Y (ITEM_TOP|ITEM_BOTTOM)
    vec3 newPos;
    vec3_zero(&newPos);

    uint32_t align_x = (align & ALIGN_X);
    uint32_t align_y = (align & ALIGN_Y);
    if (align_x == (stretchFlags & ALIGN_X) && align_x != 0)
        newPos.x = pos3.x;
    else if (align & ITEM_RIGHT)
        newPos.x = stretchItemSize.x;
    else if (!(align & ITEM_LEFT))
        newPos.x = stretchItemSize.x * 0.5f;

    if (align_y == (stretchFlags & ALIGN_Y) && align_y != 0)
        newPos.y = pos3.y;
    else if (align & ITEM_BOTTOM)
        newPos.y = stretchItemSize.y;
    else if (!(align & ITEM_TOP))
        newPos.y = stretchItemSize.y * 0.5f;
#undef ALIGN_X
#undef ALIGN_Y

    crop = startCrop;

    if (stretchFlags & ITEM_LEFT)
        crop.left += int(std::round(tl.x / scale.x));
    else if (stretchFlags & ITEM_RIGHT)
        crop.right += int(std::round((stretchItemSize.x - br.x) / scale.x));

    if (stretchFlags & ITEM_TOP)
        crop.top += int(std::round(tl.y / scale.y));
    else if (stretchFlags & ITEM_BOTTOM)
        crop.bottom += int(std::round((stretchItemSize.y - br.y) / scale.y));

    vec3_transform(&newPos, &newPos, &itemToScreen);
    newPos.x = std::round(newPos.x);
    newPos.y = std::round(newPos.y);

#if 0
    vec3 curPos;
    vec3_zero(&curPos);
    obs_sceneitem_get_pos(stretchItem, (vec2*)&curPos);
    blog(LOG_DEBUG, "curPos {%d, %d} - newPos {%d, %d}",
        int(curPos.x), int(curPos.y),
        int(newPos.x), int(newPos.y));
    blog(LOG_DEBUG, "crop {%d, %d, %d, %d}",
        crop.left, crop.top,
        crop.right, crop.bottom);
#endif

    obs_sceneitem_defer_update_begin(stretchItem);
    obs_sceneitem_set_crop(stretchItem, &crop);
    if (boundsType == OBS_BOUNDS_NONE)
        obs_sceneitem_set_pos(stretchItem, (vec2*)&newPos);
    obs_sceneitem_defer_update_end(stretchItem);
}

void ObsWindow::StretchItem(const vec2 &pos)
{
    obs_bounds_type boundsType = obs_sceneitem_get_bounds_type(stretchItem);
    uint32_t stretchFlags = (uint32_t)stretchHandle;
    bool shiftDown =  false;
    vec3 tl, br, pos3;

    vec3_zero(&tl);
    vec3_set(&br, stretchItemSize.x, stretchItemSize.y, 0.0f);

    vec3_set(&pos3, pos.x, pos.y, 0.0f);
    vec3_transform(&pos3, &pos3, &screenToItem);

    if (stretchFlags & ITEM_LEFT)
        tl.x = pos3.x;
    else if (stretchFlags & ITEM_RIGHT)
        br.x = pos3.x;

    if (stretchFlags & ITEM_TOP)
        tl.y = pos3.y;
    else if (stretchFlags & ITEM_BOTTOM)
        br.y = pos3.y;

    obs_source_t *source = obs_sceneitem_get_source(stretchItem);

    vec2 baseSize;
    vec2_set(&baseSize,
        float(obs_source_get_width(source)),
        float(obs_source_get_height(source)));

    vec2 size;
    vec2_set(&size, br.x - tl.x, br.y - tl.y);

    if (boundsType != OBS_BOUNDS_NONE) {
        if (shiftDown)
            ClampAspect(tl, br, size, baseSize);

        if (tl.x > br.x) std::swap(tl.x, br.x);
        if (tl.y > br.y) std::swap(tl.y, br.y);

        vec2_abs(&size, &size);

        obs_sceneitem_set_bounds(stretchItem, &size);
    }
    else {
        obs_sceneitem_crop crop;
        obs_sceneitem_get_crop(stretchItem, &crop);

        baseSize.x -= float(crop.left + crop.right);
        baseSize.y -= float(crop.top + crop.bottom);

        if (!shiftDown)
            ClampAspect(tl, br, size, baseSize);

        vec2_div(&size, &size, &baseSize);
        obs_sceneitem_set_scale(stretchItem, &size);
    }

    pos3 = CalculateStretchPos(tl, br);
    vec3_transform(&pos3, &pos3, &itemToScreen);

    vec2 newPos;
    vec2_set(&newPos, std::round(pos3.x), std::round(pos3.y));
    obs_sceneitem_set_pos(stretchItem, &newPos);
}

//struct SelectedItemBounds {
//    bool first = true;
//    //(top-left)  (bottom-right)
//    vec3 tl, br;
//};

//static bool AddItemBounds(obs_scene_t *scene, obs_sceneitem_t *item,
//    void *param)
//{
//    SelectedItemBounds *data = reinterpret_cast<SelectedItemBounds*>(param);
//    vec3 t[4];
//
//    auto add_bounds = [data, &t]()
//    {
//        for (const vec3 &v : t) {
//            if (data->first) {
//                vec3_copy(&data->tl, &v);
//                vec3_copy(&data->br, &v);
//                data->first = false;
//            }
//            else {
//                vec3_min(&data->tl, &data->tl, &v);
//                vec3_max(&data->br, &data->br, &v);
//            }
//        }
//    };
//
//    if (obs_sceneitem_is_group(item)) {
//        SelectedItemBounds sib;
//        obs_sceneitem_group_enum_items(item, AddItemBounds, &sib);
//
//        if (!sib.first) {
//            matrix4 xform;
//            obs_sceneitem_get_draw_transform(item, &xform);
//
//            vec3_set(&t[0], sib.tl.x, sib.tl.y, 0.0f);
//            vec3_set(&t[1], sib.tl.x, sib.br.y, 0.0f);
//            vec3_set(&t[2], sib.br.x, sib.tl.y, 0.0f);
//            vec3_set(&t[3], sib.br.x, sib.br.y, 0.0f);
//            vec3_transform(&t[0], &t[0], &xform);
//            vec3_transform(&t[1], &t[1], &xform);
//            vec3_transform(&t[2], &t[2], &xform);
//            vec3_transform(&t[3], &t[3], &xform);
//            add_bounds();
//        }
//    }
//    if (!obs_sceneitem_selected(item))
//        return true;
//
//    matrix4 boxTransform;
//    obs_sceneitem_get_box_transform(item, &boxTransform);
//
//    t[0] = GetTransformedPos(0.0f, 0.0f, boxTransform);
//    t[1] = GetTransformedPos(1.0f, 0.0f, boxTransform);
//    t[2] = GetTransformedPos(0.0f, 1.0f, boxTransform);
//    t[3] = GetTransformedPos(1.0f, 1.0f, boxTransform);
//    add_bounds();
//
//    UNUSED_PARAMETER(scene);
//    return true;
//}

//struct OffsetData {
//    float clampDist;
//    vec3 tl, br, offset;
//};

//static bool GetSourceSnapOffset(obs_scene_t *scene, obs_sceneitem_t *item,
//    void *param)
//{
//    OffsetData *data = reinterpret_cast<OffsetData*>(param);
//
//    if (obs_sceneitem_selected(item))
//        return true;
//
//    matrix4 boxTransform;
//    obs_sceneitem_get_box_transform(item, &boxTransform);
//
//    vec3 t[4] = {
//        GetTransformedPos(0.0f, 0.0f, boxTransform),
//        GetTransformedPos(1.0f, 0.0f, boxTransform),
//        GetTransformedPos(0.0f, 1.0f, boxTransform),
//        GetTransformedPos(1.0f, 1.0f, boxTransform)
//    };
//
//    bool first = true;
//    vec3 tl, br;
//    vec3_zero(&tl);
//    vec3_zero(&br);
//    for (const vec3 &v : t) {
//        if (first) {
//            vec3_copy(&tl, &v);
//            vec3_copy(&br, &v);
//            first = false;
//        }
//        else {
//            vec3_min(&tl, &tl, &v);
//            vec3_max(&br, &br, &v);
//        }
//    }
//
//    // Snap to other source edges
//#define EDGE_SNAP(l, r, x, y) \
//    do { \
//        double dist = fabsf(l.x - data->r.x); \
//        if (dist < data->clampDist && \
//            fabsf(data->offset.x) < EPSILON && \
//            data->tl.y < br.y && \
//            data->br.y > tl.y && \
//            (fabsf(data->offset.x) > dist || data->offset.x < EPSILON)) \
//            data->offset.x = l.x - data->r.x; \
//    } while (false)
//
//    EDGE_SNAP(tl, br, x, y);
//    EDGE_SNAP(tl, br, y, x);
//    EDGE_SNAP(br, tl, x, y);
//    EDGE_SNAP(br, tl, y, x);
//#undef EDGE_SNAP
//
//    UNUSED_PARAMETER(scene);
//    return true;
//}
///====吸附
//void ObsWindow::SnapItemMovement(vec2 &offset)
//{
//    OBSScene scene = ObsMain::Instance()->GetCurrentScene();
//
//    SelectedItemBounds data;
//    obs_scene_enum_items(scene, AddItemBounds, &data);
//
//    data.tl.x += offset.x;
//    data.tl.y += offset.y;
//    data.br.x += offset.x;
//    data.br.y += offset.y;
//
//    vec3 snapOffset = GetSnapOffset(data.tl, data.br);
//
//    const bool snap = config_get_bool(GetGlobalConfig(),
//        "BasicWindow", "SnappingEnabled");
//    const bool sourcesSnap = config_get_bool(GetGlobalConfig(),
//        "BasicWindow", "SourceSnapping");
//    if (snap == false)
//        return;
//    if (sourcesSnap == false) {
//        offset.x += snapOffset.x;
//        offset.y += snapOffset.y;
//        return;
//    }
//
//    const float clampDist = config_get_double(GetGlobalConfig(),
//        "BasicWindow", "SnapDistance") / m_previewScale;
//
//    OffsetData offsetData;
//    offsetData.clampDist = clampDist;
//    offsetData.tl = data.tl;
//    offsetData.br = data.br;
//    vec3_copy(&offsetData.offset, &snapOffset);
//
//    obs_scene_enum_items(scene, GetSourceSnapOffset, &offsetData);
//
//    if (fabsf(offsetData.offset.x) > EPSILON ||
//        fabsf(offsetData.offset.y) > EPSILON) {
//        offset.x += offsetData.offset.x;
//        offset.y += offsetData.offset.y;
//    }
//    else {
//        offset.x += snapOffset.x;
//        offset.y += snapOffset.y;
//    }
//}
///====获取base_width  base_height
static inline vec2 GetOBSScreenSize()
{
    obs_video_info ovi;
    vec2 size;
    vec2_zero(&size);

    if (obs_get_video_info(&ovi)) {
        size.x = float(ovi.base_width);
        size.y = float(ovi.base_height);
    }
    return size;
}

//vec3 ObsWindow::GetSnapOffset(const vec3 &tl, const vec3 &br)
//{
//    vec2 screenSize = GetOBSScreenSize();
//    vec3 clampOffset;
//
//    vec3_zero(&clampOffset);
//
//    const bool snap = config_get_bool(GetGlobalConfig(),
//        "BasicWindow", "SnappingEnabled");
//    if (snap == false)
//        return clampOffset;
//
//    const bool screenSnap = config_get_bool(GetGlobalConfig(),
//        "BasicWindow", "ScreenSnapping");
//    const bool centerSnap = config_get_bool(GetGlobalConfig(),
//        "BasicWindow", "CenterSnapping");
//
//    const float clampDist = config_get_double(GetGlobalConfig(),
//        "BasicWindow", "SnapDistance") / m_previewScale;
//    const float centerX = br.x - (br.x - tl.x) / 2.0f;
//    const float centerY = br.y - (br.y - tl.y) / 2.0f;
//
//    // Left screen edge.
//    if (screenSnap &&
//        fabsf(tl.x) < clampDist)
//        clampOffset.x = -tl.x;
//    // Right screen edge.
//    if (screenSnap &&
//        fabsf(clampOffset.x) < EPSILON &&
//        fabsf(screenSize.x - br.x) < clampDist)
//        clampOffset.x = screenSize.x - br.x;
//    // Horizontal center.
//    if (centerSnap &&
//        fabsf(screenSize.x - (br.x - tl.x)) > clampDist &&
//        fabsf(screenSize.x / 2.0f - centerX) < clampDist)
//        clampOffset.x = screenSize.x / 2.0f - centerX;
//
//    // Top screen edge.
//    if (screenSnap &&
//        fabsf(tl.y) < clampDist)
//        clampOffset.y = -tl.y;
//    // Bottom screen edge.
//    if (screenSnap &&
//        fabsf(clampOffset.y) < EPSILON &&
//        fabsf(screenSize.y - br.y) < clampDist)
//        clampOffset.y = screenSize.y - br.y;
//    // Vertical center.
//    if (centerSnap &&
//        fabsf(screenSize.y - (br.y - tl.y)) > clampDist &&
//        fabsf(screenSize.y / 2.0f - centerY) < clampDist)
//        clampOffset.y = screenSize.y / 2.0f - centerY;
//
//    return clampOffset;
//}

static bool move_items(obs_scene_t *scene, obs_sceneitem_t *item, void *param)
{
    if (obs_sceneitem_locked(item))
        return true;

    bool selected = obs_sceneitem_selected(item);
    vec2 *offset = reinterpret_cast<vec2*>(param);

    if (obs_sceneitem_is_group(item) && !selected) {
        matrix4 transform;
        vec3 new_offset;
        vec3_set(&new_offset, offset->x, offset->y, 0.0f);

        obs_sceneitem_get_draw_transform(item, &transform);
        vec4_set(&transform.t, 0.0f, 0.0f, 0.0f, 1.0f);
        matrix4_inv(&transform, &transform);
        vec3_transform(&new_offset, &new_offset, &transform);
        obs_sceneitem_group_enum_items(item, move_items, &new_offset);
    }

    if (selected) {
        vec2 pos;
        obs_sceneitem_get_pos(item, &pos);
        vec2_add(&pos, &pos, offset);
        obs_sceneitem_set_pos(item, &pos);
    }

    UNUSED_PARAMETER(scene);
    return true;
}
///pos是在base空间 
void ObsWindow::MoveItems(const vec2 &pos)
{
    OBSScene scene = ObsMain::Instance()->GetCurrentScene();
    ///按下鼠标时的坐标与当前的位置的差值
    vec2 offset;
    ///拖动鼠标时的坐标与上一次的位置的差值
    vec2 moveOffset;
    vec2_sub(&offset, &pos, &startPos);
    vec2_sub(&moveOffset, &offset, &lastMoveOffset);

    //如果吸附调用
//    if(!CheckKeyState(StateControl))
//        SnapItemMovement(moveOffset);

    vec2_add(&lastMoveOffset, &lastMoveOffset, &moveOffset);
    obs_scene_enum_items(scene, move_items, &moveOffset);
}

void ObsWindow::OnMouseMoveEvent(ObsMouseEvent *event){
    if (m_locked)
        return;
    if (mouseDown) {
        ///pos 在 base_width  base_height 坐标系中
        vec2 pos = GetMouseEventPos(event);
        if (!mouseMoved && !mouseOverItems &&  stretchHandle == ItemHandle::None) {
            ProcessClick(startPos);
            mouseOverItems = SelectedAtPos(startPos);
        }

        pos.x = std::round(pos.x);
        pos.y = std::round(pos.y);

        ///拖动触点
        if (stretchHandle != ItemHandle::None) {
            OBSScene scene = ObsMain::Instance()->GetCurrentScene();
            obs_sceneitem_t *group = obs_sceneitem_get_group(
                scene, stretchItem);
            if (group) {
                vec3 group_pos;
                vec3_set(&group_pos, pos.x, pos.y, 0.0f);
                vec3_transform(&group_pos, &group_pos,
                    &invGroupTransform);
                pos.x = group_pos.x;
                pos.y = group_pos.y;
            }

            if (cropping)
                CropItem(pos);
            else
                StretchItem(pos);
        }else if (mouseOverItems) {
            MoveItems(pos);
        }
        mouseMoved = true;
    }
}
void ObsWindow::OnMouseReleaseEvent(ObsMouseEvent *event)
{
    if (m_locked) {
        return;
    }

    if (mouseDown || rmouseDown) {
        vec2 pos = GetMouseEventPos(event);

        if (!mouseMoved)
        {
            if(mouseDown)
                DoSelect(pos);
            else
            {
                OBSSceneItem item = GetSelectItemAtPos(pos);
                if (!item)
                {
                    DoSelect(pos);
                }
            }
        }

        if (stretchGroup) {
            obs_sceneitem_defer_group_resize_end(stretchGroup);
        }

        stretchItem = nullptr;
        stretchGroup = nullptr;
        mouseDown = false;
        rmouseDown = false;
        mouseMoved = false;
        cropping = false;

        if (event->button == RightButton)
        {
            ObsMain::Instance()->DoShowMenu(GetWndHandle(),ObsPoint(event->x, event->y));
        }
    }
}
void ObsWindow::OnFocusChange(bool bGet)
{
    if (bGet)
        m_locked = false;
    else
        m_locked = true;
}


void ObsWindow::OnDropFile(const char* file)
{
    ObsMain::Instance()->AddSourceFile(file);
}



