/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

#include "../util/bmem.h"
#include "input.h"
#ifdef __APPLE__
#include <objc/objc-runtime.h>
#endif

/*
 * This is an API-independent graphics subsystem wrapper.
 *
 *   This allows the use of OpenGL and different Direct3D versions through
 * one shared interface.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define GS_MAX_TEXTURES 8

struct vec2;
struct vec3;
struct vec4;
struct quat;
struct axisang;
struct plane;
struct matrix3;
struct matrix4;

///绘制模式
enum gs_draw_mode {
	GS_POINTS,      ///以点的形式
	GS_LINES,       ///以线段的形式
	GS_LINESTRIP,   ///线性线段
	GS_TRIS,        ///三角形
	GS_TRISTRIP,    ///线性三角形
};


#pragma mark ----颜色空间跟颜色格式是两个不同的概念 example:(RGB颜色格式 可能是RGB(256*256*256大小空间) 也可能是sRGB(小于<256*256*256空间))

///GPU层面的颜色
enum gs_color_format {
	GS_UNKNOWN,
	GS_A8,           //只有一个 Alpha 通道，8位。
	GS_R8,           //只有一个红色通道，8位。
	GS_RGBA,         //四通道：红、绿、蓝、Alpha，每通道8位。
	GS_BGRX,         //四通道：蓝、绿、红，加一个未使用的通道，每通道8位。
	GS_BGRA,         //四通道：蓝、绿、红、Alpha，每通道8位。
	GS_R10G10B10A2,  //四通道：红、绿、蓝10位，Alpha 2位。
	GS_RGBA16,       //四通道：红、绿、蓝、Alpha，每通道16位。
	GS_R16,          //单通道红色，16位。
	GS_RGBA16F,      //四通道：红、绿、蓝、Alpha，每通道16位浮点。
	GS_RGBA32F,      //四通道：红、绿、蓝、Alpha，每通道32位浮点。
	GS_RG16F,        //双通道：红、绿，每通道16位浮点
	GS_RG32F,        //双通道：红、绿，每通道32位浮点
	GS_R16F,         //单通道红色，16位浮点。
	GS_R32F,         //单通道红色，32位浮点。
	GS_DXT1,         //压缩格式，主要用于RGB纹理。
	GS_DXT3,         //压缩格式，用于具有显式alpha的纹理
	GS_DXT5,         //压缩格式，用于具有插值alpha的纹理。
	GS_R8G8,         //双通道：红、绿，每通道8位。
	GS_RGBA_UNORM,   //标准归一化的RGBA格式，每通道8位。
	GS_BGRX_UNORM,   //标准归一化的BGRX格式，每通道8位。
	GS_BGRA_UNORM,   //标准归一化的BGRA格式，每通道8位。
	GS_RG16,         //双通道：红、绿，每通道16位。
};

enum gs_color_space {
	GS_CS_SRGB,         /* SDR  (Standard Dynamic Range, SDR) 8bits  */
	GS_CS_SRGB_16F,     /* High-precision SDR */
	GS_CS_709_EXTENDED, /* Canvas, Mac EDR (HDR) High Dynamic Range（高动态范围） */
	GS_CS_709_SCRGB,    /* 1.0 = 80 nits, Windows/Linux HDR */
};

enum gs_zstencil_format {
	GS_ZS_NONE,
	GS_Z16,
	GS_Z24_S8,
	GS_Z32F,
	GS_Z32F_S8X24,
};

enum gs_index_type {
	GS_UNSIGNED_SHORT,
	GS_UNSIGNED_LONG,
};
/*
 glCullFace 是 OpenGL 中的一个函数,用于控制背面剔除(backface culling)的行为。
 背面剔除是一种优化技术,它可以通过忽略不可见的面来提高渲染性能。当启用背面剔除时,OpenGL 会自动删除那些面向相反方向的三角形,从而减少需要处理的几何体数量。
 glCullFace 函数用于指定要剔除的面的方向。它接受以下三个参数之一:
 GL_FRONT: 剔除正面(front-facing)的三角形。
 GL_BACK: 剔除背面(back-facing)的三角形。
 GL_FRONT_AND_BACK: 同时剔除正面和背面的三角形。
 */
enum gs_cull_mode {
	GS_BACK,
	GS_FRONT,
	GS_NEITHER,
};
///混合因子
enum gs_blend_type {
	GS_BLEND_ZERO,
	GS_BLEND_ONE,
	GS_BLEND_SRCCOLOR,
	GS_BLEND_INVSRCCOLOR,
	GS_BLEND_SRCALPHA,
	GS_BLEND_INVSRCALPHA,
	GS_BLEND_DSTCOLOR,
	GS_BLEND_INVDSTCOLOR,
	GS_BLEND_DSTALPHA,
	GS_BLEND_INVDSTALPHA,
	GS_BLEND_SRCALPHASAT,
};

enum gs_blend_op_type {
	GS_BLEND_OP_ADD,
	GS_BLEND_OP_SUBTRACT,
	GS_BLEND_OP_REVERSE_SUBTRACT,
	GS_BLEND_OP_MIN,
	GS_BLEND_OP_MAX
};

enum gs_depth_test {
	GS_NEVER,
	GS_LESS,
	GS_LEQUAL,
	GS_EQUAL,
	GS_GEQUAL,
	GS_GREATER,
	GS_NOTEQUAL,
	GS_ALWAYS,
};

enum gs_stencil_side {
	GS_STENCIL_FRONT = 1,
	GS_STENCIL_BACK,
	GS_STENCIL_BOTH,
};

enum gs_stencil_op_type {
	GS_KEEP,
	GS_ZERO,
	GS_REPLACE,
	GS_INCR,
	GS_DECR,
	GS_INVERT,
};

enum gs_cube_sides {
	GS_POSITIVE_X,
	GS_NEGATIVE_X,
	GS_POSITIVE_Y,
	GS_NEGATIVE_Y,
	GS_POSITIVE_Z,
	GS_NEGATIVE_Z,
};
///纹理采样滤镜 glTextureParami
enum gs_sample_filter {
	GS_FILTER_POINT,
	GS_FILTER_LINEAR,
	GS_FILTER_ANISOTROPIC,
	GS_FILTER_MIN_MAG_POINT_MIP_LINEAR,
	GS_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
	GS_FILTER_MIN_POINT_MAG_MIP_LINEAR,
	GS_FILTER_MIN_LINEAR_MAG_MIP_POINT,
	GS_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
	GS_FILTER_MIN_MAG_LINEAR_MIP_POINT,
};
//图形采样中的纹理寻址模式。这些寻址模式定义了当纹理坐标超出 [0, 1] 范围时如何处理纹理的边界
/*
 case GS_ADDRESS_WRAP:
     return GL_REPEAT;
 case GS_ADDRESS_CLAMP:
     return GL_CLAMP_TO_EDGE;
 case GS_ADDRESS_MIRROR:
     return GL_MIRRORED_REPEAT;
 case GS_ADDRESS_BORDER:
     return GL_CLAMP_TO_BORDER;
 case GS_ADDRESS_MIRRORONCE:
     return GL_MIRROR_CLAMP_EXT;
 */
enum gs_address_mode {

    /*
     当纹理坐标超出 [0, 1] 范围时,会使用纹理边缘像素的颜色。
     这可以避免纹理边缘出现重复或失真的问题。
     */
	GS_ADDRESS_CLAMP,
    /*
     当纹理坐标超出 [0, 1] 范围时,纹理会被重复平铺。
     这是默认的包装模式
     */
	GS_ADDRESS_WRAP,
    /*
     当纹理坐标超出 [0, 1] 范围时,纹理会被镜像重复平铺。
     即在 [0, 1] 范围内正常显示,在 [-1, 0] 和 [1, 2] 范围内镜像显示。
     */
	GS_ADDRESS_MIRROR,
    /*
     当纹理坐标超出 [0, 1] 范围时,会使用设置的边缘颜色。
     需要通过 glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor) 设置边缘颜色。
     */
	GS_ADDRESS_BORDER,
    /*
     这是一个扩展常量,需要检查支持情况。
     当纹理坐标超出 [0, 1] 范围时,会使用最近的纹理边缘像素的颜色。
     这可以避免纹理边缘出现重复或失真的问题,但不会产生镜像效果。
     */
	GS_ADDRESS_MIRRORONCE,
};

enum gs_texture_type {
	GS_TEXTURE_2D,
	GS_TEXTURE_3D,
	GS_TEXTURE_CUBE,
};

struct gs_device_loss {
	void (*device_loss_release)(void *data);
	void (*device_loss_rebuild)(void *device, void *data);
	void *data;
};

struct gs_monitor_info {
	int rotation_degrees;
	long x;
	long y;
	long cx;
	long cy;
};
//纹理坐标数据
struct gs_tvertarray {
	size_t width; //纹理坐标的维度
	void *array;  //纹理坐标数据
};
///内存顶点数据
struct gs_vb_data {
    //坐标数量
	size_t num;
	struct vec3 *points;    //位置
	struct vec3 *normals;   //法线
	struct vec3 *tangents;  //切线
	uint32_t *colors;       //颜色

    //纹理数量
	size_t num_tex;
    //纹理坐标数据
	struct gs_tvertarray *tvarray;
};

static inline struct gs_vb_data *gs_vbdata_create(void)
{
	return (struct gs_vb_data *)bzalloc(sizeof(struct gs_vb_data));
}

static inline void gs_vbdata_destroy(struct gs_vb_data *data)
{
	uint32_t i;
	if (!data)
		return;

	bfree(data->points);
	bfree(data->normals);
	bfree(data->tangents);
	bfree(data->colors);
	for (i = 0; i < data->num_tex; i++)
		bfree(data->tvarray[i].array);
	bfree(data->tvarray);
	bfree(data);
}

struct gs_sampler_info {
	enum gs_sample_filter filter;
	enum gs_address_mode address_u;
	enum gs_address_mode address_v;
	enum gs_address_mode address_w;
	int max_anisotropy;
	uint32_t border_color;
};

struct gs_display_mode {
	uint32_t width;
	uint32_t height;
	uint32_t bits;
	uint32_t freq;
};

struct gs_rect {
	int x;
	int y;
	int cx;
	int cy;
};

/* wrapped opaque data types */

struct gs_texture;
struct gs_stage_surface;
struct gs_zstencil_buffer;
struct gs_vertex_buffer;
struct gs_index_buffer;
struct gs_sampler_state;
struct gs_shader;
struct gs_swap_chain;
struct gs_timer;
struct gs_texrender;
struct gs_shader_param;
struct gs_effect;
struct gs_effect_technique;
struct gs_effect_pass;
struct gs_effect_param;
struct gs_device;
struct graphics_subsystem;

typedef struct gs_texture gs_texture_t;
typedef struct gs_stage_surface gs_stagesurf_t;
typedef struct gs_zstencil_buffer gs_zstencil_t;
typedef struct gs_vertex_buffer gs_vertbuffer_t;
typedef struct gs_index_buffer gs_indexbuffer_t;
typedef struct gs_sampler_state gs_samplerstate_t;
typedef struct gs_swap_chain gs_swapchain_t;
typedef struct gs_timer gs_timer_t;
typedef struct gs_timer_range gs_timer_range_t;
typedef struct gs_texture_render gs_texrender_t;
typedef struct gs_shader gs_shader_t;
typedef struct gs_shader_param gs_sparam_t;
typedef struct gs_effect gs_effect_t;
typedef struct gs_effect_technique gs_technique_t;
typedef struct gs_effect_pass gs_epass_t;
typedef struct gs_effect_param gs_eparam_t;
typedef struct gs_device gs_device_t;
typedef struct graphics_subsystem graphics_t;

/* ---------------------------------------------------
 * shader functions
 * --------------------------------------------------- */
///着色器的参数类型
enum gs_shader_param_type {
	GS_SHADER_PARAM_UNKNOWN,
	GS_SHADER_PARAM_BOOL,
	GS_SHADER_PARAM_FLOAT,
	GS_SHADER_PARAM_INT,
	GS_SHADER_PARAM_STRING,
	GS_SHADER_PARAM_VEC2,
	GS_SHADER_PARAM_VEC3,
	GS_SHADER_PARAM_VEC4,
	GS_SHADER_PARAM_INT2,
	GS_SHADER_PARAM_INT3,
	GS_SHADER_PARAM_INT4,
	GS_SHADER_PARAM_MATRIX4X4,
	GS_SHADER_PARAM_TEXTURE,
};

struct gs_shader_texture {
	gs_texture_t *tex;
	bool srgb;
};

#ifndef SWIG
///着色器参数信息（包括参数的类型和名称）
struct gs_shader_param_info {
	enum gs_shader_param_type type;
	const char *name;
};
///着色器类型
enum gs_shader_type {
	GS_SHADER_VERTEX,  //顶点
	GS_SHADER_PIXEL,   //片段
};
#pragma mark ---  着色器相关
EXPORT void gs_shader_destroy(gs_shader_t *shader);

//着色器参数相关
EXPORT int gs_shader_get_num_params(const gs_shader_t *shader);
EXPORT gs_sparam_t *gs_shader_get_param_by_idx(gs_shader_t *shader,
					       uint32_t param);
EXPORT gs_sparam_t *gs_shader_get_param_by_name(gs_shader_t *shader,
						const char *name);

//view矩阵 world矩阵
EXPORT gs_sparam_t *gs_shader_get_viewproj_matrix(const gs_shader_t *shader);
EXPORT gs_sparam_t *gs_shader_get_world_matrix(const gs_shader_t *shader);

EXPORT void gs_shader_get_param_info(const gs_sparam_t *param,
				     struct gs_shader_param_info *info);
EXPORT void gs_shader_set_bool(gs_sparam_t *param, bool val);
EXPORT void gs_shader_set_float(gs_sparam_t *param, float val);
EXPORT void gs_shader_set_int(gs_sparam_t *param, int val);
EXPORT void gs_shader_set_matrix3(gs_sparam_t *param,
				  const struct matrix3 *val);
EXPORT void gs_shader_set_matrix4(gs_sparam_t *param,
				  const struct matrix4 *val);
EXPORT void gs_shader_set_vec2(gs_sparam_t *param, const struct vec2 *val);
EXPORT void gs_shader_set_vec3(gs_sparam_t *param, const struct vec3 *val);
EXPORT void gs_shader_set_vec4(gs_sparam_t *param, const struct vec4 *val);
EXPORT void gs_shader_set_texture(gs_sparam_t *param, gs_texture_t *val);
EXPORT void gs_shader_set_val(gs_sparam_t *param, const void *val, size_t size);
EXPORT void gs_shader_set_default(gs_sparam_t *param);
EXPORT void gs_shader_set_next_sampler(gs_sparam_t *param,
				       gs_samplerstate_t *sampler);

#endif

/* ---------------------------------------------------
 * effect functions
 * --------------------------------------------------- */

/*enum gs_effect_property_type {
	GS_EFFECT_NONE,
	GS_EFFECT_BOOL,
	GS_EFFECT_FLOAT,
	GS_EFFECT_COLOR,
	GS_EFFECT_TEXTURE
};*/

#ifndef SWIG
///effect文件中 参数的描述信息
struct gs_effect_param_info {
	const char *name;
	enum gs_shader_param_type type;

	/* const char *full_name;
	enum gs_effect_property_type prop_type;

	float min, max, inc, mul; */
};
#endif


#pragma mark --- effect 相关

EXPORT void gs_effect_destroy(gs_effect_t *effect);
EXPORT gs_technique_t *gs_effect_get_technique(const gs_effect_t *effect,
					       const char *name);
EXPORT gs_technique_t *
gs_effect_get_current_technique(const gs_effect_t *effect);
///切换当前的technique及effect 为technique和technique关联的effect
EXPORT size_t gs_technique_begin(gs_technique_t *technique);
///结束当前的technique及effect 为technique和technique关联的effect
EXPORT void gs_technique_end(gs_technique_t *technique);

///切换effect的当前pass级pass下对应的vs、fs
EXPORT bool gs_technique_begin_pass(gs_technique_t *technique, size_t pass);
EXPORT bool gs_technique_begin_pass_by_name(gs_technique_t *technique,
                                            const char *name);
EXPORT void gs_technique_end_pass(gs_technique_t *technique);
EXPORT gs_epass_t *gs_technique_get_pass_by_idx(const gs_technique_t *technique,
                                                size_t pass);
EXPORT gs_epass_t *gs_technique_get_pass_by_name(const gs_technique_t *technique,
                                                 const char *name);
//effect参数
EXPORT size_t gs_effect_get_num_params(const gs_effect_t *effect);
EXPORT gs_eparam_t *gs_effect_get_param_by_idx(const gs_effect_t *effect,
                                               size_t param);
EXPORT gs_eparam_t *gs_effect_get_param_by_name(const gs_effect_t *effect,
                                                const char *name);
EXPORT size_t gs_param_get_num_annotations(const gs_eparam_t *param);
EXPORT gs_eparam_t *gs_param_get_annotation_by_idx(const gs_eparam_t *param,
                                                   size_t annotation);
EXPORT gs_eparam_t *gs_param_get_annotation_by_name(const gs_eparam_t *param,
                                                    const char *name);

//默认切换到effect当前的technique及pass
EXPORT bool gs_effect_loop(gs_effect_t *effect, const char *name);

/** 把effect的参数值提交到  shader的参数上*/
EXPORT void gs_effect_update_params(gs_effect_t *effect);

//view矩阵 world矩阵
EXPORT gs_eparam_t *gs_effect_get_viewproj_matrix(const gs_effect_t *effect);
EXPORT gs_eparam_t *gs_effect_get_world_matrix(const gs_effect_t *effect);



#ifndef SWIG
EXPORT void gs_effect_get_param_info(const gs_eparam_t *param,
				     struct gs_effect_param_info *info);
#endif

//给effet 设置参数值
EXPORT void gs_effect_set_bool(gs_eparam_t *param, bool val);
EXPORT void gs_effect_set_float(gs_eparam_t *param, float val);
EXPORT void gs_effect_set_int(gs_eparam_t *param, int val);
EXPORT void gs_effect_set_matrix4(gs_eparam_t *param,
                                  const struct matrix4 *val);
EXPORT void gs_effect_set_vec2(gs_eparam_t *param, const struct vec2 *val);
EXPORT void gs_effect_set_vec3(gs_eparam_t *param, const struct vec3 *val);
EXPORT void gs_effect_set_vec4(gs_eparam_t *param, const struct vec4 *val);
EXPORT void gs_effect_set_texture(gs_eparam_t *param, gs_texture_t *val);
EXPORT void gs_effect_set_texture_srgb(gs_eparam_t *param, gs_texture_t *val);
EXPORT void gs_effect_set_val(gs_eparam_t *param, const void *val, size_t size);
EXPORT void gs_effect_set_default(gs_eparam_t *param);
EXPORT size_t gs_effect_get_val_size(gs_eparam_t *param);
EXPORT void *gs_effect_get_val(gs_eparam_t *param);
EXPORT size_t gs_effect_get_default_val_size(gs_eparam_t *param);
EXPORT void *gs_effect_get_default_val(gs_eparam_t *param);
EXPORT void gs_effect_set_next_sampler(gs_eparam_t *param,
				       gs_samplerstate_t *sampler);
EXPORT void gs_effect_set_color(gs_eparam_t *param, uint32_t argb);

///纹理渲染
EXPORT gs_texrender_t *gs_texrender_create(enum gs_color_format format,
                                           enum gs_zstencil_format zsformat);
EXPORT void gs_texrender_destroy(gs_texrender_t *texrender);
EXPORT bool gs_texrender_begin(gs_texrender_t *texrender, uint32_t cx,
                               uint32_t cy);
EXPORT bool gs_texrender_begin_with_color_space(gs_texrender_t *texrender,
                                                uint32_t cx, uint32_t cy,
                                                enum gs_color_space space);
EXPORT void gs_texrender_end(gs_texrender_t *texrender);

///rendered置为false代表需下一次重新渲染 
EXPORT void gs_texrender_reset(gs_texrender_t *texrender);
EXPORT gs_texture_t *gs_texrender_get_texture(const gs_texrender_t *texrender);
EXPORT enum gs_color_format gs_texrender_get_format(const gs_texrender_t *texrender);



/* ---------------------------------------------------
 * graphics subsystem
 * --------------------------------------------------- */

#define GS_BUILD_MIPMAPS (1 << 0)
#define GS_DYNAMIC (1 << 1)
#define GS_RENDER_TARGET (1 << 2)
#define GS_GL_DUMMYTEX (1 << 3) /**<< texture with no allocated texture data */
#define GS_DUP_BUFFER \
	(1 << 4) /**<< do not pass buffer ownership when
				 *    creating a vertex/index buffer */
#define GS_SHARED_TEX (1 << 5)
#define GS_SHARED_KM_TEX (1 << 6)

/* ---------------- */
/* global functions */

#define GS_SUCCESS 0
#define GS_ERROR_FAIL -1
#define GS_ERROR_MODULE_NOT_FOUND -2
#define GS_ERROR_NOT_SUPPORTED -3

struct gs_window {
#if defined(_WIN32)
	void *hwnd;
#elif defined(__APPLE__)
	__unsafe_unretained id view;
#elif defined(__linux__) || defined(__FreeBSD__)
	/* I'm not sure how portable defining id to uint32_t is. */
	uint32_t id;
	void *display;
#endif
};

struct gs_init_data {
	struct gs_window window;
	uint32_t cx, cy;
	uint32_t num_backbuffers;
	enum gs_color_format format;
	enum gs_zstencil_format zsformat;
	uint32_t adapter;
};

#define GS_DEVICE_OPENGL 1
#define GS_DEVICE_DIRECT3D_11 2

//一些常量
EXPORT const char *gs_get_device_name(void);
EXPORT int gs_get_device_type(void);
EXPORT void gs_enum_adapters(bool (*callback)(void *param, const char *name,
					      uint32_t id),
                             void *param);

///创建 graphics_t 对象
EXPORT int gs_create(graphics_t **graphics, const char *module,
                     uint32_t adapter);
EXPORT void gs_destroy(graphics_t *graphics);



///enter跟leave是成对出现的 为了保证数据的唯一性
EXPORT void gs_enter_context(graphics_t *graphics);
EXPORT void gs_leave_context(void);


EXPORT graphics_t *gs_get_context(void);
EXPORT void *gs_get_device_obj(void);

//矩阵相关
EXPORT void gs_matrix_push(void);
EXPORT void gs_matrix_pop(void);
EXPORT void gs_matrix_identity(void);
EXPORT void gs_matrix_transpose(void);
EXPORT void gs_matrix_set(const struct matrix4 *matrix);
EXPORT void gs_matrix_get(struct matrix4 *dst);
EXPORT void gs_matrix_mul(const struct matrix4 *matrix);
EXPORT void gs_matrix_rotquat(const struct quat *rot);
EXPORT void gs_matrix_rotaa(const struct axisang *rot);
EXPORT void gs_matrix_translate(const struct vec3 *pos);
EXPORT void gs_matrix_scale(const struct vec3 *scale);
EXPORT void gs_matrix_rotaa4f(float x, float y, float z, float angle);
EXPORT void gs_matrix_translate3f(float x, float y, float z);
EXPORT void gs_matrix_scale3f(float x, float y, float z);

#pragma mark -- 顶点数据
//创建顶点缓冲数据或者把缓存的顶点数据恢复到顶点缓冲中 准备绘制
EXPORT void gs_render_start(bool b_new);
//绘制
EXPORT void gs_render_stop(enum gs_draw_mode mode);
//暂时不用绘制  暂时保存顶点数据
EXPORT gs_vertbuffer_t *gs_render_save(void);
EXPORT void gs_vertex2f(float x, float y);
EXPORT void gs_vertex3f(float x, float y, float z);
EXPORT void gs_normal3f(float x, float y, float z);
EXPORT void gs_color(uint32_t color);
EXPORT void gs_texcoord(float x, float y, int unit);
EXPORT void gs_vertex2v(const struct vec2 *v);
EXPORT void gs_vertex3v(const struct vec3 *v);
EXPORT void gs_normal3v(const struct vec3 *v);
EXPORT void gs_color4v(const struct vec4 *v);
EXPORT void gs_texcoord2v(const struct vec2 *v, int unit);

///=== ?
EXPORT input_t *gs_get_input(void);



#pragma mark --- effect及顶点和片段着色器
EXPORT gs_effect_t *gs_get_effect(void);
EXPORT gs_effect_t *gs_effect_create_from_file(const char *file,
                                               char **error_string);
EXPORT gs_effect_t *gs_effect_create(const char *effect_string,
                                     const char *filename, char **error_string);
EXPORT gs_shader_t *gs_vertexshader_create_from_file(const char *file,
                                                     char **error_string);
EXPORT gs_shader_t *gs_pixelshader_create_from_file(const char *file,
                                                    char **error_string);



#pragma mark--- 获取纹理图片得数据
enum gs_image_alpha_mode {
	GS_IMAGE_ALPHA_STRAIGHT,
	GS_IMAGE_ALPHA_PREMULTIPLY_SRGB,
	GS_IMAGE_ALPHA_PREMULTIPLY,
};

EXPORT gs_texture_t *gs_texture_create_from_file(const char *file);
EXPORT uint8_t *gs_create_texture_file_data(const char *file,
					    enum gs_color_format *format,
					    uint32_t *cx, uint32_t *cy);
EXPORT uint8_t *gs_create_texture_file_data2(
                                             const char *file, enum gs_image_alpha_mode alpha_mode,
                                             enum gs_color_format *format, uint32_t *cx, uint32_t *cy);
EXPORT uint8_t *gs_create_texture_file_data3(const char *file,
			     enum gs_image_alpha_mode alpha_mode,
			     enum gs_color_format *format, uint32_t *cx,
                             uint32_t *cy, enum gs_color_space *space);

#define GS_FLIP_U (1 << 0)
#define GS_FLIP_V (1 << 1)


#pragma mark--- 绘制
/**
 * Draws a 2D sprite
 *
 *   If width or height is 0, the width or height of the texture will be used.
 * The flip value specifies whether the texture should be flipped on the U or V
 * axis with GS_FLIP_U and GS_FLIP_V.
 *
 */
EXPORT void gs_draw_sprite(gs_texture_t *tex, uint32_t flip, uint32_t width,
			   uint32_t height);

EXPORT void gs_draw_sprite_subregion(gs_texture_t *tex, uint32_t flip,
				     uint32_t x, uint32_t y, uint32_t cx,
				     uint32_t cy);

EXPORT void gs_draw_cube_backdrop(gs_texture_t *cubetex, const struct quat *rot,
				  float left, float right, float top,
				  float bottom, float znear);


#pragma mark --- 视口相关
/** sets the viewport to current swap chain size */
EXPORT void gs_reset_viewport(void);
/** sets default screen-sized orthographic mode */
EXPORT void gs_set_2d_mode(void);
/** sets default screen-sized perspective mode */
EXPORT void gs_set_3d_mode(double fovy, double znear, double zvar);

//在开始绘制之前 保存当前swpchain的视口大小
EXPORT void gs_viewport_push(void);
//结束绘制时恢复之前保存的视口
EXPORT void gs_viewport_pop(void);



#pragma mark --- 给纹理设置图片数据
EXPORT void gs_texture_set_image(gs_texture_t *tex, const uint8_t *data,
				 uint32_t linesize, bool invert);
EXPORT void gs_cubetexture_set_image(gs_texture_t *cubetex, uint32_t side,
				     const void *data, uint32_t linesize,
				     bool invert);

//透视
EXPORT void gs_perspective(float fovy, float aspect, float znear, float zfar);


#pragma mark ----混合模式
//绘制前保存当前的混合模式
EXPORT void gs_blend_state_push(void);
//绘制后恢复之前保存的混合模式
EXPORT void gs_blend_state_pop(void);
EXPORT void gs_reset_blend_state(void);


#pragma mark ---当前swapchain相关
///每一张画布会对应一个swapchain 当有新的画布时会新建swapchain
EXPORT gs_swapchain_t *gs_swapchain_create(const struct gs_init_data *data);
///更新当前swapchain中帧缓冲的大小
EXPORT void gs_resize(uint32_t x, uint32_t y);
EXPORT void gs_update_color_space(void);
EXPORT void gs_get_size(uint32_t *x, uint32_t *y);
EXPORT uint32_t gs_get_width(void);
EXPORT uint32_t gs_get_height(void);



#pragma mark ---- 创建纹理对象  
EXPORT gs_texture_t *gs_texture_create(uint32_t width, uint32_t height,
				       enum gs_color_format color_format,
				       uint32_t levels, const uint8_t **data,
				       uint32_t flags);
EXPORT gs_texture_t *gs_cubetexture_create(uint32_t size, enum gs_color_format color_format,
		      uint32_t levels, const uint8_t **data, uint32_t flags);
EXPORT gs_texture_t *gs_voltexture_create(uint32_t width, uint32_t height,
					  uint32_t depth,
					  enum gs_color_format color_format,
					  uint32_t levels, const uint8_t **data,
					  uint32_t flags);

#pragma mark ---深度模板相关
EXPORT gs_zstencil_t *gs_zstencil_create(uint32_t width, uint32_t height,
					 enum gs_zstencil_format format);

#pragma mark ---暂存相关
EXPORT gs_stagesurf_t *gs_stagesurface_create(uint32_t width, uint32_t height,
		       enum gs_color_format color_format);

#pragma mark ---采样器相关
EXPORT gs_samplerstate_t *gs_samplerstate_create(const struct gs_sampler_info *info);


#pragma mark --- 创建着色器
EXPORT gs_shader_t *gs_vertexshader_create(const char *shader, const char *file,
					   char **error_string);
EXPORT gs_shader_t *gs_pixelshader_create(const char *shader, const char *file,
					  char **error_string);

#pragma mark --- 创建顶点、索引缓冲
EXPORT gs_vertbuffer_t *gs_vertexbuffer_create(struct gs_vb_data *data,
					       uint32_t flags);
EXPORT gs_indexbuffer_t *gs_indexbuffer_create(enum gs_index_type type,
					       void *indices, size_t num,
					       uint32_t flags);

EXPORT gs_timer_t *gs_timer_create(void);
///===?
EXPORT gs_timer_range_t *gs_timer_range_create(void);

#pragma mark---加载数据
EXPORT enum gs_texture_type gs_get_texture_type(const gs_texture_t *texture);
EXPORT void gs_load_vertexbuffer(gs_vertbuffer_t *vertbuffer);
EXPORT void gs_load_indexbuffer(gs_indexbuffer_t *indexbuffer);
EXPORT void gs_load_texture(gs_texture_t *tex, int unit);
EXPORT void gs_load_samplerstate(gs_samplerstate_t *samplerstate, int unit);
EXPORT void gs_load_vertexshader(gs_shader_t *vertshader);
EXPORT void gs_load_pixelshader(gs_shader_t *pixelshader);
EXPORT void gs_load_default_samplerstate(bool b_3d, int unit);

///
EXPORT gs_shader_t *gs_get_vertex_shader(void);
EXPORT gs_shader_t *gs_get_pixel_shader(void);
EXPORT enum gs_color_space gs_get_color_space(void);
EXPORT gs_texture_t *gs_get_render_target(void);
EXPORT gs_zstencil_t *gs_get_zstencil_target(void);

#pragma mark --- 设置渲染缓冲区
EXPORT void gs_set_render_target(gs_texture_t *tex, gs_zstencil_t *zstencil);
EXPORT void gs_set_render_target_with_color_space(gs_texture_t *tex,
                                                  gs_zstencil_t *zstencil,
                                                  enum gs_color_space space);
EXPORT void gs_set_cube_render_target(gs_texture_t *cubetex, int side,
                                      gs_zstencil_t *zstencil);


#pragma mark --- 设置sRGB
EXPORT void gs_enable_framebuffer_srgb(bool enable);
EXPORT bool gs_framebuffer_srgb_enabled(void);
EXPORT bool gs_get_linear_srgb(void);
EXPORT bool gs_set_linear_srgb(bool linear_srgb);


#pragma mark --- copy或暂存纹理
EXPORT void gs_copy_texture(gs_texture_t *dst, gs_texture_t *src);
EXPORT void gs_copy_texture_region(gs_texture_t *dst, uint32_t dst_x,
				   uint32_t dst_y, gs_texture_t *src,
				   uint32_t src_x, uint32_t src_y,
				   uint32_t src_w, uint32_t src_h);
EXPORT void gs_stage_texture(gs_stagesurf_t *dst, gs_texture_t *src);


#pragma mark --- 绘制
EXPORT void gs_begin_frame(void);
EXPORT void gs_begin_scene(void);//解绑所有的纹理绑定 
EXPORT void gs_draw(enum gs_draw_mode draw_mode, uint32_t start_vert,
                    uint32_t num_verts);
EXPORT void gs_end_scene(void);
#define GS_CLEAR_COLOR (1 << 0)
#define GS_CLEAR_DEPTH (1 << 1)
#define GS_CLEAR_STENCIL (1 << 2)
EXPORT void gs_load_swapchain(gs_swapchain_t *swapchain);
EXPORT void gs_clear(uint32_t clear_flags, const struct vec4 *color,
                     float depth, uint8_t stencil);
EXPORT bool gs_is_present_ready(void);
EXPORT void gs_present(void);
///在 OpenGL 中,渲染命令并不是立即执行的。相反,它们会被缓存在内存中,等待后续的处理和执行。这种延迟执行的机制可以提高性能,因为 OpenGL 可以对这些命令进行优化和批处理。但是,在某些情况下,我们可能需要确保之前的所有 OpenGL 命令都已经被立即执行,
EXPORT void gs_flush(void);
EXPORT void gs_set_cull_mode(enum gs_cull_mode mode);
EXPORT enum gs_cull_mode gs_get_cull_mode(void);


#pragma mark --- 混合、深度测试、模板
EXPORT void gs_enable_blending(bool enable);
EXPORT void gs_enable_depth_test(bool enable);
EXPORT void gs_enable_stencil_test(bool enable);
EXPORT void gs_enable_stencil_write(bool enable);
EXPORT void gs_enable_color(bool red, bool green, bool blue, bool alpha);
EXPORT void gs_blend_function(enum gs_blend_type src, enum gs_blend_type dest);
EXPORT void gs_blend_function_separate(enum gs_blend_type src_c,
				       enum gs_blend_type dest_c,
				       enum gs_blend_type src_a,
				       enum gs_blend_type dest_a);
EXPORT void gs_blend_op(enum gs_blend_op_type op);
EXPORT void gs_depth_function(enum gs_depth_test test);
EXPORT void gs_stencil_function(enum gs_stencil_side side,
				enum gs_depth_test test);
EXPORT void gs_stencil_op(enum gs_stencil_side side,
			  enum gs_stencil_op_type fail,
			  enum gs_stencil_op_type zfail,
			  enum gs_stencil_op_type zpass);

///视口、裁剪
EXPORT void gs_set_viewport(int x, int y, int width, int height);
EXPORT void gs_get_viewport(struct gs_rect *rect);
EXPORT void gs_set_scissor_rect(const struct gs_rect *rect);
//正交投影矩阵
EXPORT void gs_ortho(float left, float right, float top, float bottom,
		     float znear, float zfar);
//透视投影
EXPORT void gs_frustum(float left, float right, float top, float bottom,
		       float znear, float zfar);
EXPORT void gs_projection_push(void);
EXPORT void gs_projection_pop(void);
EXPORT void gs_swapchain_destroy(gs_swapchain_t *swapchain);


#pragma mark --- 纹理相关操作
EXPORT void gs_texture_destroy(gs_texture_t *tex);
EXPORT uint32_t gs_texture_get_width(const gs_texture_t *tex);
EXPORT uint32_t gs_texture_get_height(const gs_texture_t *tex);
EXPORT enum gs_color_format gs_texture_get_color_format(const gs_texture_t *tex);
EXPORT bool gs_texture_map(gs_texture_t *tex, uint8_t **ptr,
                           uint32_t *linesize);
EXPORT void gs_texture_unmap(gs_texture_t *tex);
/** special-case function (GL only) - specifies whether the texture is a
 * GL_TEXTURE_RECTANGLE type, which doesn't use normalized texture
 * coordinates, doesn't support mipmapping, and requires address clamping */
EXPORT bool gs_texture_is_rect(const gs_texture_t *tex);
/**
 * Gets a pointer to the context-specific object associated with the texture.
 * For example, for GL, this is a GLuint*.  For D3D11, ID3D11Texture2D*.
 */
EXPORT void *gs_texture_get_obj(gs_texture_t *tex);
EXPORT void gs_cubetexture_destroy(gs_texture_t *cubetex);
EXPORT uint32_t gs_cubetexture_get_size(const gs_texture_t *cubetex);
EXPORT enum gs_color_format gs_cubetexture_get_color_format(const gs_texture_t *cubetex);
///===?
EXPORT void gs_voltexture_destroy(gs_texture_t *voltex);
EXPORT uint32_t gs_voltexture_get_width(const gs_texture_t *voltex);
EXPORT uint32_t gs_voltexture_get_height(const gs_texture_t *voltex);
EXPORT uint32_t gs_voltexture_get_depth(const gs_texture_t *voltex);
EXPORT enum gs_color_format gs_voltexture_get_color_format(const gs_texture_t *voltex);

#pragma mark ----暂存缓存区
EXPORT void gs_stagesurface_destroy(gs_stagesurf_t *stagesurf);
EXPORT uint32_t gs_stagesurface_get_width(const gs_stagesurf_t *stagesurf);
EXPORT uint32_t gs_stagesurface_get_height(const gs_stagesurf_t *stagesurf);
EXPORT enum gs_color_format gs_stagesurface_get_color_format(const gs_stagesurf_t *stagesurf);
EXPORT bool gs_stagesurface_map(gs_stagesurf_t *stagesurf, uint8_t **data, uint32_t *linesize);
EXPORT void gs_stagesurface_unmap(gs_stagesurf_t *stagesurf);
EXPORT void gs_zstencil_destroy(gs_zstencil_t *zstencil);


EXPORT void gs_samplerstate_destroy(gs_samplerstate_t *samplerstate);
EXPORT void gs_vertexbuffer_destroy(gs_vertbuffer_t *vertbuffer);

#pragma mark ----顶点数据相关
EXPORT void gs_vertexbuffer_flush(gs_vertbuffer_t *vertbuffer);
EXPORT void gs_vertexbuffer_flush_direct(gs_vertbuffer_t *vertbuffer,
                                         const struct gs_vb_data *data);
EXPORT struct gs_vb_data *gs_vertexbuffer_get_data(const gs_vertbuffer_t *vertbuffer);
EXPORT void gs_indexbuffer_destroy(gs_indexbuffer_t *indexbuffer);
EXPORT void gs_indexbuffer_flush(gs_indexbuffer_t *indexbuffer);
EXPORT void gs_indexbuffer_flush_direct(gs_indexbuffer_t *indexbuffer,
					const void *data);
EXPORT void *gs_indexbuffer_get_data(const gs_indexbuffer_t *indexbuffer);
EXPORT size_t gs_indexbuffer_get_num_indices(const gs_indexbuffer_t *indexbuffer);
EXPORT enum gs_index_type gs_indexbuffer_get_type(const gs_indexbuffer_t *indexbuffer);


#pragma mark --- time
//GL_TIMESTAMP,用于获取当前的时间戳。存储到timer->queries[0]对像中
EXPORT void gs_timer_destroy(gs_timer_t *timer);
EXPORT void gs_timer_begin(gs_timer_t *timer);
EXPORT void gs_timer_end(gs_timer_t *timer);
EXPORT bool gs_timer_get_data(gs_timer_t *timer, uint64_t *ticks);
EXPORT void gs_timer_range_destroy(gs_timer_range_t *timer);
EXPORT void gs_timer_range_begin(gs_timer_range_t *range);
EXPORT void gs_timer_range_end(gs_timer_range_t *range);
EXPORT bool gs_timer_range_get_data(gs_timer_range_t *range, bool *disjoint, uint64_t *frequency);
EXPORT bool gs_nv12_available(void);
EXPORT bool gs_p010_available(void);
EXPORT bool gs_is_monitor_hdr(void *monitor);

#pragma mark -----debug
#define GS_USE_DEBUG_MARKERS 0
#if GS_USE_DEBUG_MARKERS
static const float GS_DEBUG_COLOR_DEFAULT[] = {0.5f, 0.5f, 0.5f, 1.0f};
static const float GS_DEBUG_COLOR_RENDER_VIDEO[] = {0.0f, 0.5f, 0.0f, 1.0f};
static const float GS_DEBUG_COLOR_MAIN_TEXTURE[] = {0.0f, 0.25f, 0.0f, 1.0f};
static const float GS_DEBUG_COLOR_DISPLAY[] = {0.0f, 0.5f, 0.5f, 1.0f};
static const float GS_DEBUG_COLOR_SOURCE[] = {0.0f, 0.5f, 5.0f, 1.0f};
static const float GS_DEBUG_COLOR_ITEM[] = {0.5f, 0.0f, 0.0f, 1.0f};
static const float GS_DEBUG_COLOR_ITEM_TEXTURE[] = {0.25f, 0.0f, 0.0f, 1.0f};
static const float GS_DEBUG_COLOR_CONVERT_FORMAT[] = {0.5f, 0.5f, 0.0f, 1.0f};
#define GS_DEBUG_MARKER_BEGIN(color, markername) \
	gs_debug_marker_begin(color, markername)
#define GS_DEBUG_MARKER_BEGIN_FORMAT(color, format, ...) \
	gs_debug_marker_begin_format(color, format, __VA_ARGS__)
#define GS_DEBUG_MARKER_END() gs_debug_marker_end()
#else
#define GS_DEBUG_MARKER_BEGIN(color, markername) ((void)0)
#define GS_DEBUG_MARKER_BEGIN_FORMAT(color, format, ...) ((void)0)
#define GS_DEBUG_MARKER_END() ((void)0)
#endif

EXPORT void gs_debug_marker_begin(const float color[4], const char *markername);
EXPORT void gs_debug_marker_begin_format(const float color[4],
					 const char *format, ...);
EXPORT void gs_debug_marker_end(void);

#ifdef __APPLE__

/** platform specific function for creating (GL_TEXTURE_RECTANGLE) textures
 * from shared surface resources */
EXPORT gs_texture_t *gs_texture_create_from_iosurface(void *iosurf);
EXPORT bool gs_texture_rebind_iosurface(gs_texture_t *texture, void *iosurf);
EXPORT gs_texture_t *gs_texture_open_shared(uint32_t handle);
EXPORT bool gs_shared_texture_available(void);

#elif _WIN32

EXPORT bool gs_gdi_texture_available(void);
EXPORT bool gs_shared_texture_available(void);

struct gs_duplicator;
typedef struct gs_duplicator gs_duplicator_t;

/**
 * Gets information about the monitor at the specific index, returns false
 * when there is no monitor at the specified index
 */
EXPORT bool
gs_get_duplicator_monitor_info(int monitor_idx,
			       struct gs_monitor_info *monitor_info);

EXPORT int gs_duplicator_get_monitor_index(void *monitor);

/** creates a windows 8+ output duplicator (monitor capture) */
EXPORT gs_duplicator_t *gs_duplicator_create(int monitor_idx);
EXPORT void gs_duplicator_destroy(gs_duplicator_t *duplicator);

EXPORT bool gs_duplicator_update_frame(gs_duplicator_t *duplicator);
EXPORT gs_texture_t *gs_duplicator_get_texture(gs_duplicator_t *duplicator);
EXPORT enum gs_color_space
gs_duplicator_get_color_space(gs_duplicator_t *duplicator);
EXPORT float gs_duplicator_get_sdr_white_level(gs_duplicator_t *duplicator);

EXPORT uint32_t gs_get_adapter_count(void);

/** creates a windows GDI-lockable texture */
EXPORT gs_texture_t *gs_texture_create_gdi(uint32_t width, uint32_t height);

EXPORT void *gs_texture_get_dc(gs_texture_t *gdi_tex);
EXPORT void gs_texture_release_dc(gs_texture_t *gdi_tex);

/** creates a windows shared texture from a texture handle */
EXPORT gs_texture_t *gs_texture_open_shared(uint32_t handle);
EXPORT gs_texture_t *gs_texture_open_nt_shared(uint32_t handle);

#define GS_INVALID_HANDLE (uint32_t) - 1
EXPORT uint32_t gs_texture_get_shared_handle(gs_texture_t *tex);

EXPORT gs_texture_t *gs_texture_wrap_obj(void *obj);

#define GS_WAIT_INFINITE (uint32_t) - 1

/**
 * acquires a lock on a keyed mutex texture.
 * returns -1 on generic failure, ETIMEDOUT if timed out
 */
EXPORT int gs_texture_acquire_sync(gs_texture_t *tex, uint64_t key,
				   uint32_t ms);

/**
 * releases a lock on a keyed mutex texture to another device.
 * return 0 on success, -1 on error
 */
EXPORT int gs_texture_release_sync(gs_texture_t *tex, uint64_t key);

EXPORT bool gs_texture_create_nv12(gs_texture_t **tex_y, gs_texture_t **tex_uv,
				   uint32_t width, uint32_t height,
				   uint32_t flags);
EXPORT bool gs_texture_create_p010(gs_texture_t **tex_y, gs_texture_t **tex_uv,
				   uint32_t width, uint32_t height,
				   uint32_t flags);

EXPORT gs_stagesurf_t *gs_stagesurface_create_nv12(uint32_t width,
						   uint32_t height);
EXPORT gs_stagesurf_t *gs_stagesurface_create_p010(uint32_t width,
						   uint32_t height);

EXPORT void gs_register_loss_callbacks(const struct gs_device_loss *callbacks);
EXPORT void gs_unregister_loss_callbacks(void *data);

#elif defined(__linux__) || defined(__FreeBSD__) || defined(__DragonFly__)

EXPORT gs_texture_t *gs_texture_create_from_dmabuf(
	unsigned int width, unsigned int height, uint32_t drm_format,
	enum gs_color_format color_format, uint32_t n_planes, const int *fds,
	const uint32_t *strides, const uint32_t *offsets,
	const uint64_t *modifiers);

enum gs_dmabuf_flags {
	GS_DMABUF_FLAG_NONE = 0,
	GS_DMABUF_FLAG_IMPLICIT_MODIFIERS_SUPPORTED = (1 << 0),
};

EXPORT bool gs_query_dmabuf_capabilities(enum gs_dmabuf_flags *dmabuf_flags,
					 uint32_t **drm_formats,
					 size_t *n_formats);

EXPORT bool gs_query_dmabuf_modifiers_for_format(uint32_t drm_format,
						 uint64_t **modifiers,
						 size_t *n_modifiers);

EXPORT gs_texture_t *
gs_texture_create_from_pixmap(uint32_t width, uint32_t height,
			      enum gs_color_format color_format,
			      uint32_t target, void *pixmap);

#endif

/* inline functions used by modules */
static inline uint32_t gs_get_format_bpp(enum gs_color_format format)
{
	switch (format) {
	case GS_DXT1:
		return 4;
	case GS_A8:
	case GS_R8:
	case GS_DXT3:
	case GS_DXT5:
		return 8;
	case GS_R16:
	case GS_R16F:
	case GS_R8G8:
		return 16;
	case GS_RGBA:
	case GS_BGRX:
	case GS_BGRA:
	case GS_R10G10B10A2:
	case GS_RG16F:
	case GS_R32F:
	case GS_RGBA_UNORM:
	case GS_BGRX_UNORM:
	case GS_BGRA_UNORM:
	case GS_RG16:
		return 32;
	case GS_RGBA16:
	case GS_RGBA16F:
	case GS_RG32F:
		return 64;
	case GS_RGBA32F:
		return 128;
	case GS_UNKNOWN:
		return 0;
	}

	return 0;
}

static inline bool gs_is_compressed_format(enum gs_color_format format)
{
	return (format == GS_DXT1 || format == GS_DXT3 || format == GS_DXT5);
}

static inline bool gs_is_srgb_format(enum gs_color_format format)
{
	switch (format) {
	case GS_RGBA:
	case GS_BGRX:
	case GS_BGRA:
		return true;
	default:
		return false;
	}
}

static inline enum gs_color_format
gs_generalize_format(enum gs_color_format format)
{
	switch (format) {
	case GS_RGBA_UNORM:
		return GS_RGBA;
	case GS_BGRX_UNORM:
		return GS_BGRX;
	case GS_BGRA_UNORM:
		return GS_BGRA;
	default:
		return format;
	}
}

static inline enum gs_color_format gs_get_format_from_space(enum gs_color_space space)
{
	switch (space) {
	case GS_CS_SRGB:
		break;
	case GS_CS_SRGB_16F:
	case GS_CS_709_EXTENDED:
	case GS_CS_709_SCRGB:
		return GS_RGBA16F;
	}

	return GS_RGBA;
}

static inline uint32_t gs_get_total_levels(uint32_t width, uint32_t height,
					   uint32_t depth)
{
	uint32_t size = width > height ? width : height;
	size = size > depth ? size : depth;
	uint32_t num_levels = 1;

	while (size > 1) {
		size /= 2;
		num_levels++;
	}

	return num_levels;
}

#ifdef __cplusplus
}
#endif
