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

#include "../util/threading.h"
#include "../util/darray.h"
#include "graphics.h"
#include "matrix3.h"
#include "matrix4.h"

struct gs_exports {
    ///设备名称
	const char *(*device_get_name)(void);
    ///设备类型 opengl/3d311
	int (*device_get_type)(void);
	bool (*device_enum_adapters)(bool (*callback)(void *, const char *,
						      uint32_t),
				     void *);
    ///预处理器名称
	const char *(*device_preprocessor_name)(void);
    ///创建当前的渲染设备
	int (*device_create)(gs_device_t **device, uint32_t adapter);
	void (*device_destroy)(gs_device_t *device);
    
    ///enter跟leave是成对出现的 为了保证数据的唯一性
	void (*device_enter_context)(gs_device_t *device);
	void (*device_leave_context)(gs_device_t *device);
    
    ///当前平台的opengl OBJ
	void *(*device_get_device_obj)(gs_device_t *device);
    
    ///每一个display会对应一个gs_swapchain_t 渲染线程会遍历所有的display
	gs_swapchain_t *(*device_swapchain_create)(
		gs_device_t *device, const struct gs_init_data *data);
    
    //当前displaysize有变化
	void (*device_resize)(gs_device_t *device, uint32_t x, uint32_t y);
    
	enum gs_color_space (*device_get_color_space)(gs_device_t *device);
	void (*device_update_color_space)(gs_device_t *device);
    
	void (*device_get_size)(const gs_device_t *device, uint32_t *x,
				uint32_t *y);
	uint32_t (*device_get_width)(const gs_device_t *device);
	uint32_t (*device_get_height)(const gs_device_t *device);
    
    ///创建二维纹理
	gs_texture_t *(*device_texture_create)(
		gs_device_t *device, uint32_t width, uint32_t height,
		enum gs_color_format color_format, uint32_t levels,
		const uint8_t **data, uint32_t flags);
    
    ///创建立方体纹理
	gs_texture_t *(*device_cubetexture_create)(
		gs_device_t *device, uint32_t size,
		enum gs_color_format color_format, uint32_t levels,
		const uint8_t **data, uint32_t flags);
    
    ///创建三维纹理
	gs_texture_t *(*device_voltexture_create)(
		gs_device_t *device, uint32_t width, uint32_t height,
		uint32_t depth, enum gs_color_format color_format,
		uint32_t levels, const uint8_t *const *data, uint32_t flags);
    
    ///深度模版缓冲 (跟帧缓冲相关)
	gs_zstencil_t *(*device_zstencil_create)(
		gs_device_t *device, uint32_t width, uint32_t height,
		enum gs_zstencil_format format);
    
    //创建临时存储区域
	gs_stagesurf_t *(*device_stagesurface_create)(
		gs_device_t *device, uint32_t width, uint32_t height,
		enum gs_color_format color_format);
    
    //创建着色器中使用的纹理采样器
	gs_samplerstate_t *(*device_samplerstate_create)(
		gs_device_t *device, const struct gs_sampler_info *info);
    
    //顶点着色器
	gs_shader_t *(*device_vertexshader_create)(gs_device_t *device,
						   const char *shader,
						   const char *file,
						   char **error_string);
    //片段着色器
	gs_shader_t *(*device_pixelshader_create)(gs_device_t *device,
						  const char *shader,
						  const char *file,
						  char **error_string);
    
    //顶点缓冲区
	gs_vertbuffer_t *(*device_vertexbuffer_create)(gs_device_t *device,
						       struct gs_vb_data *data,
						       uint32_t flags);
    //索引缓冲区
	gs_indexbuffer_t *(*device_indexbuffer_create)(gs_device_t *device,
						       enum gs_index_type type,
						       void *indices,
						       size_t num,
						       uint32_t flags);
    //创建性能查询对象
	gs_timer_t *(*device_timer_create)(gs_device_t *device);
	gs_timer_range_t *(*device_timer_range_create)(gs_device_t *device);
    
    //当前纹理的类型
    enum gs_texture_type (*device_get_texture_type)(
		const gs_texture_t *texture);
   
    //顶点 索引 纹理
	void (*device_load_vertexbuffer)(gs_device_t *device,
					 gs_vertbuffer_t *vertbuffer);
	void (*device_load_indexbuffer)(gs_device_t *device,
					gs_indexbuffer_t *indexbuffer);
	void (*device_load_texture)(gs_device_t *device, gs_texture_t *tex,
				    int unit);
    
    
    // 采样器  定点着色器 片段着色器 深度模版
	void (*device_load_samplerstate)(gs_device_t *device,
					 gs_samplerstate_t *samplerstate,
					 int unit);
    //device的vs设置为vertshader
	void (*device_load_vertexshader)(gs_device_t *device,
					 gs_shader_t *vertshader);
    //device的ps设置为pixelshader
	void (*device_load_pixelshader)(gs_device_t *device,
					gs_shader_t *pixelshader);
    //==?
	void (*device_load_default_samplerstate)(gs_device_t *device, bool b_3d,
						 int unit);
	gs_shader_t *(*device_get_vertex_shader)(const gs_device_t *device);
	gs_shader_t *(*device_get_pixel_shader)(const gs_device_t *device);
	gs_texture_t *(*device_get_render_target)(const gs_device_t *device);
	gs_zstencil_t *(*device_get_zstencil_target)(const gs_device_t *device);
    
    //设置device的当前帧缓冲
	void (*device_set_render_target)(gs_device_t *device, gs_texture_t *tex,
					 gs_zstencil_t *zstencil);
	void (*device_set_render_target_with_color_space)(
		gs_device_t *device, gs_texture_t *tex, gs_zstencil_t *zstencil,
		enum gs_color_space space);
	void (*device_set_cube_render_target)(gs_device_t *device,
					      gs_texture_t *cubetex, int side,
					      gs_zstencil_t *zstencil);
    
    /*
     颜色一致性: 如果你的应用程序使用了线性颜色空间来进行光照计算和颜色混合，使用 sRGB 渲染可以确保在最终显示时，颜色显得更自然和一致。
     适用于纹理: 在使用 sRGB 纹理时，确保启用 GL_FRAMEBUFFER_SRGB 可以使得纹理的颜色在渲染到屏幕上时自动进行 gamma 校正。
     
     标准srgb的Gamma校正（人眼对亮度的感知是非线性的,也就是说人眼对亮度变化的响应并不是线性的。在低亮度下,人眼对亮度变化更敏感。
     Gamma校正的过程就是通过一个幂函数对亮度值进行非线性变换,以更好地匹配人眼和设备的非线性特性。通常使用下面的公式进行
     */
	void (*device_enable_framebuffer_srgb)(gs_device_t *device,
					       bool enable);
    //是否启用GL_FRAMEBUFFER_SRGB
	bool (*device_framebuffer_srgb_enabled)(gs_device_t *device);
	void (*device_copy_texture)(gs_device_t *device, gs_texture_t *dst,
				    gs_texture_t *src);
    
    //拷贝纹理
	void (*device_copy_texture_region)(gs_device_t *device,
					   gs_texture_t *dst, uint32_t dst_x,
					   uint32_t dst_y, gs_texture_t *src,
					   uint32_t src_x, uint32_t src_y,
					   uint32_t src_w, uint32_t src_h);
    
    //纹理数据暂存到 GL_PIXEL_PACK_BUFFER缓冲区
	void (*device_stage_texture)(gs_device_t *device, gs_stagesurf_t *dst,
				     gs_texture_t *src);
    
	void (*device_begin_frame)(gs_device_t *device);
    ///解除纹理绑定
	void (*device_begin_scene)(gs_device_t *device);
    
    //绘制
	void (*device_draw)(gs_device_t *device, enum gs_draw_mode draw_mode,
			    uint32_t start_vert, uint32_t num_verts);
    //
	void (*device_end_scene)(gs_device_t *device);
    //设置swaphchain为当前swaphchain
	void (*device_load_swapchain)(gs_device_t *device,
				      gs_swapchain_t *swaphchain);
    
    //绘制前清空
	void (*device_clear)(gs_device_t *device, uint32_t clear_flags,
			     const struct vec4 *color, float depth,
			     uint8_t stencil);
    //===
	bool (*device_is_present_ready)(gs_device_t *device);
    
    //绘制缓冲区的数据到屏幕上
	void (*device_present)(gs_device_t *device);
	void (*device_flush)(gs_device_t *device);
    
    //设置模剔除式
	void (*device_set_cull_mode)(gs_device_t *device,
				     enum gs_cull_mode mode);
	enum gs_cull_mode (*device_get_cull_mode)(const gs_device_t *device);
    
    // 一些功能(模版测试 混合 深度测试)
	void (*device_enable_blending)(gs_device_t *device, bool enable);
	void (*device_enable_depth_test)(gs_device_t *device, bool enable);
	void (*device_enable_stencil_test)(gs_device_t *device, bool enable);
	void (*device_enable_stencil_write)(gs_device_t *device, bool enable);
	void (*device_enable_color)(gs_device_t *device, bool red, bool green,
				    bool blue, bool alpha);
	void (*device_blend_function)(gs_device_t *device,
				      enum gs_blend_type src,
				      enum gs_blend_type dest);
	void (*device_blend_function_separate)(gs_device_t *device,
					       enum gs_blend_type src_c,
					       enum gs_blend_type dest_c,
					       enum gs_blend_type src_a,
					       enum gs_blend_type dest_a);
	void (*device_blend_op)(gs_device_t *device, enum gs_blend_op_type op);

	void (*device_depth_function)(gs_device_t *device,
				      enum gs_depth_test test);
	void (*device_stencil_function)(gs_device_t *device,
					enum gs_stencil_side side,
					enum gs_depth_test test);
	void (*device_stencil_op)(gs_device_t *device,
				  enum gs_stencil_side side,
				  enum gs_stencil_op_type fail,
				  enum gs_stencil_op_type zfail,
				  enum gs_stencil_op_type zpass);
    
    
    
    //视口
	void (*device_set_viewport)(gs_device_t *device, int x, int y,
				    int width, int height);
	void (*device_get_viewport)(const gs_device_t *device,
				    struct gs_rect *rect);
    
    //裁剪
	void (*device_set_scissor_rect)(gs_device_t *device,
					const struct gs_rect *rect);
    //正交
	void (*device_ortho)(gs_device_t *device, float left, float right,
			     float top, float bottom, float znear, float zfar);
    //设置透视投影
	void (*device_frustum)(gs_device_t *device, float left, float right,
			       float top, float bottom, float znear,
			       float zfar);
    
    ///透视矩阵栈
	void (*device_projection_push)(gs_device_t *device);
	void (*device_projection_pop)(gs_device_t *device);

    
    
    
    
    //释放交换链和纹理
	void (*gs_swapchain_destroy)(gs_swapchain_t *swapchain);
	void (*gs_texture_destroy)(gs_texture_t *tex);

    //纹理宽高
    uint32_t (*gs_texture_get_width)(const gs_texture_t *tex);
	uint32_t (*gs_texture_get_height)(const gs_texture_t *tex);
    
    //纹理格式
	enum gs_color_format (*gs_texture_get_color_format)(
		const gs_texture_t *tex);
    
    /*2D纹理相关操作*/
    
    //将纹理数据映射到内存中
	bool (*gs_texture_map)(gs_texture_t *tex, uint8_t **ptr,
			       uint32_t *linesize);
	void (*gs_texture_unmap)(gs_texture_t *tex);
	bool (*gs_texture_is_rect)(const gs_texture_t *tex);
	void *(*gs_texture_get_obj)(const gs_texture_t *tex);

    
    /*3D纹理相关操作*/
	void (*gs_cubetexture_destroy)(gs_texture_t *cubetex);
	uint32_t (*gs_cubetexture_get_size)(const gs_texture_t *cubetex);
	enum gs_color_format (*gs_cubetexture_get_color_format)(
		const gs_texture_t *cubetex);

    //体积纹理
	void (*gs_voltexture_destroy)(gs_texture_t *voltex);
	uint32_t (*gs_voltexture_get_width)(const gs_texture_t *voltex);
	uint32_t (*gs_voltexture_get_height)(const gs_texture_t *voltex);
	uint32_t (*gs_voltexture_get_depth)(const gs_texture_t *voltex);
	enum gs_color_format (*gs_voltexture_get_color_format)(
		const gs_texture_t *voltex);

    //暂存缓冲区 用于快速读取GPU数据
	void (*gs_stagesurface_destroy)(gs_stagesurf_t *stagesurf);
	uint32_t (*gs_stagesurface_get_width)(const gs_stagesurf_t *stagesurf);
	uint32_t (*gs_stagesurface_get_height)(const gs_stagesurf_t *stagesurf);
	enum gs_color_format (*gs_stagesurface_get_color_format)(
		const gs_stagesurf_t *stagesurf);
	bool (*gs_stagesurface_map)(gs_stagesurf_t *stagesurf, uint8_t **data,
				    uint32_t *linesize);
	void (*gs_stagesurface_unmap)(gs_stagesurf_t *stagesurf);

    ///渲染缓冲对象
	void (*gs_zstencil_destroy)(gs_zstencil_t *zstencil);
	void (*gs_samplerstate_destroy)(gs_samplerstate_t *samplerstate);

    ///顶点缓冲VBO
    void (*gs_vertexbuffer_destroy)(gs_vertbuffer_t *vertbuffer);
	void (*gs_vertexbuffer_flush)(gs_vertbuffer_t *vertbuffer);
	void (*gs_vertexbuffer_flush_direct)(gs_vertbuffer_t *vertbuffer,
					     const struct gs_vb_data *data);
	struct gs_vb_data *(*gs_vertexbuffer_get_data)(
		const gs_vertbuffer_t *vertbuffer);
    
    ///顶点索引缓冲EBO
	void (*gs_indexbuffer_destroy)(gs_indexbuffer_t *indexbuffer);
	void (*gs_indexbuffer_flush)(gs_indexbuffer_t *indexbuffer);
	void (*gs_indexbuffer_flush_direct)(gs_indexbuffer_t *indexbuffer,
					    const void *data);
	void *(*gs_indexbuffer_get_data)(const gs_indexbuffer_t *indexbuffer);
	size_t (*gs_indexbuffer_get_num_indices)(
		const gs_indexbuffer_t *indexbuffer);
	enum gs_index_type (*gs_indexbuffer_get_type)(
		const gs_indexbuffer_t *indexbuffer);

    //glQueryCounter
	void (*gs_timer_destroy)(gs_timer_t *timer);
	void (*gs_timer_begin)(gs_timer_t *timer);
	void (*gs_timer_end)(gs_timer_t *timer);
	bool (*gs_timer_get_data)(gs_timer_t *timer, uint64_t *ticks);
    
    ///opengl未实现
	void (*gs_timer_range_destroy)(gs_timer_range_t *range);
	bool (*gs_timer_range_begin)(gs_timer_range_t *range);
	bool (*gs_timer_range_end)(gs_timer_range_t *range);
	bool (*gs_timer_range_get_data)(gs_timer_range_t *range, bool *disjoint,
					uint64_t *frequency);

    ///着色器
	void (*gs_shader_destroy)(gs_shader_t *shader);
	int (*gs_shader_get_num_params)(const gs_shader_t *shader);
	gs_sparam_t *(*gs_shader_get_param_by_idx)(gs_shader_t *shader,
						   uint32_t param);
	gs_sparam_t *(*gs_shader_get_param_by_name)(gs_shader_t *shader,
						    const char *name);
	gs_sparam_t *(*gs_shader_get_viewproj_matrix)(const gs_shader_t *shader);
	gs_sparam_t *(*gs_shader_get_world_matrix)(const gs_shader_t *shader);
	void (*gs_shader_get_param_info)(const gs_sparam_t *param,
					 struct gs_shader_param_info *info);
	void (*gs_shader_set_bool)(gs_sparam_t *param, bool val);
	void (*gs_shader_set_float)(gs_sparam_t *param, float val);
	void (*gs_shader_set_int)(gs_sparam_t *param, int val);
	void (*gs_shader_set_matrix3)(gs_sparam_t *param,
				      const struct matrix3 *val);
	void (*gs_shader_set_matrix4)(gs_sparam_t *param,
				      const struct matrix4 *val);
	void (*gs_shader_set_vec2)(gs_sparam_t *param, const struct vec2 *val);
	void (*gs_shader_set_vec3)(gs_sparam_t *param, const struct vec3 *val);
	void (*gs_shader_set_vec4)(gs_sparam_t *param, const struct vec4 *val);
	void (*gs_shader_set_texture)(gs_sparam_t *param, gs_texture_t *val);
	void (*gs_shader_set_val)(gs_sparam_t *param, const void *val,
				  size_t size);
	void (*gs_shader_set_default)(gs_sparam_t *param);
	void (*gs_shader_set_next_sampler)(gs_sparam_t *param,
					   gs_samplerstate_t *sampler);

    
    
    
	bool (*device_nv12_available)(gs_device_t *device);
	bool (*device_p010_available)(gs_device_t *device);

	bool (*device_is_monitor_hdr)(gs_device_t *device, void *monitor);

	void (*device_debug_marker_begin)(gs_device_t *device,
					  const char *markername,
					  const float color[4]);
	void (*device_debug_marker_end)(gs_device_t *device);

#ifdef __APPLE__
	/* OSX/Cocoa specific functions */
    ///高效地在不同的应用程序或不同的系统组件之间共享图像数据。它允许不同的进程或线程访问同一块内存区域，以便进行图像处理、渲染或其他需要共享数据的操作。
	gs_texture_t *(*device_texture_create_from_iosurface)(gs_device_t *dev,
							      void *iosurf);
	gs_texture_t *(*device_texture_open_shared)(gs_device_t *dev,
						    uint32_t handle);
	bool (*gs_texture_rebind_iosurface)(gs_texture_t *texture,
					    void *iosurf);
	bool (*device_shared_texture_available)(void);

#elif _WIN32
	bool (*device_gdi_texture_available)(void);
	bool (*device_shared_texture_available)(void);

	bool (*device_get_duplicator_monitor_info)(
		gs_device_t *device, int monitor_idx,
		struct gs_monitor_info *monitor_info);
	int (*device_duplicator_get_monitor_index)(gs_device_t *device,
						   void *monitor);

	gs_duplicator_t *(*device_duplicator_create)(gs_device_t *device,
						     int monitor_idx);
	void (*gs_duplicator_destroy)(gs_duplicator_t *duplicator);

	bool (*gs_duplicator_update_frame)(gs_duplicator_t *duplicator);
	gs_texture_t *(*gs_duplicator_get_texture)(gs_duplicator_t *duplicator);
	enum gs_color_space (*gs_duplicator_get_color_space)(
		gs_duplicator_t *duplicator);
	float (*gs_duplicator_get_sdr_white_level)(gs_duplicator_t *duplicator);

	uint32_t (*gs_get_adapter_count)(void);

	gs_texture_t *(*device_texture_create_gdi)(gs_device_t *device,
						   uint32_t width,
						   uint32_t height);

	void *(*gs_texture_get_dc)(gs_texture_t *gdi_tex);
	void (*gs_texture_release_dc)(gs_texture_t *gdi_tex);

	gs_texture_t *(*device_texture_open_shared)(gs_device_t *device,
						    uint32_t handle);
	gs_texture_t *(*device_texture_open_nt_shared)(gs_device_t *device,
						       uint32_t handle);
	uint32_t (*device_texture_get_shared_handle)(gs_texture_t *tex);
	gs_texture_t *(*device_texture_wrap_obj)(gs_device_t *device,
						 void *obj);
	int (*device_texture_acquire_sync)(gs_texture_t *tex, uint64_t key,
					   uint32_t ms);
	int (*device_texture_release_sync)(gs_texture_t *tex, uint64_t key);
	bool (*device_texture_create_nv12)(gs_device_t *device,
					   gs_texture_t **tex_y,
					   gs_texture_t **tex_uv,
					   uint32_t width, uint32_t height,
					   uint32_t flags);
	bool (*device_texture_create_p010)(gs_device_t *device,
					   gs_texture_t **tex_y,
					   gs_texture_t **tex_uv,
					   uint32_t width, uint32_t height,
					   uint32_t flags);

	gs_stagesurf_t *(*device_stagesurface_create_nv12)(gs_device_t *device,
							   uint32_t width,
							   uint32_t height);
	gs_stagesurf_t *(*device_stagesurface_create_p010)(gs_device_t *device,
							   uint32_t width,
							   uint32_t height);
	void (*device_register_loss_callbacks)(
		gs_device_t *device, const struct gs_device_loss *callbacks);
	void (*device_unregister_loss_callbacks)(gs_device_t *device,
						 void *data);
#elif defined(__linux__) || defined(__FreeBSD__) || defined(__DragonFly__)
	struct gs_texture *(*device_texture_create_from_dmabuf)(
		gs_device_t *device, unsigned int width, unsigned int height,
		uint32_t drm_format, enum gs_color_format color_format,
		uint32_t n_planes, const int *fds, const uint32_t *strides,
		const uint32_t *offsets, const uint64_t *modifiers);
	bool (*device_query_dmabuf_capabilities)(
		gs_device_t *device, enum gs_dmabuf_flags *dmabuf_flags,
		uint32_t **drm_formats, size_t *n_formats);
	bool (*device_query_dmabuf_modifiers_for_format)(gs_device_t *device,
							 uint32_t drm_format,
							 uint64_t **modifiers,
							 size_t *n_modifiers);
	struct gs_texture *(*device_texture_create_from_pixmap)(
		gs_device_t *device, uint32_t width, uint32_t height,
		enum gs_color_format color_format, uint32_t target,
		void *pixmap);
#endif
};
///源和目标的混合因子
struct blend_state {
	bool enabled;
	enum gs_blend_type src_c;
	enum gs_blend_type dest_c;
	enum gs_blend_type src_a;
	enum gs_blend_type dest_a;
	enum gs_blend_op_type op;
};
///此对象全局只有一个
struct graphics_subsystem {
    ///当前渲染引擎库的对象
	void *module;
	gs_device_t *device;
	struct gs_exports exports;
    // 此处的窗口大小和变换矩阵之所以是数组 是因为可能会在不同的graphics之间切换
	DARRAY(struct gs_rect) viewport_stack;
	DARRAY(struct matrix4) matrix_stack;
    
	size_t cur_matrix;

	struct matrix4 projection;
    //当前着色器相关的effect
	struct gs_effect *cur_effect;
    ///OpenGL的顶点相关的缓冲对象
	gs_vertbuffer_t *sprite_buffer;
    ///更新数据后是否立即绘制(更新数据后有时需要)
	bool using_immediate;
    //需要暂时缓存起来的顶点数据
	struct gs_vb_data *vbd;
	gs_vertbuffer_t *immediate_vertbuffer;

    //需要立即绘制的顶点数据
	DARRAY(struct vec3) verts; //位置
	DARRAY(struct vec3) norms; //法线
	DARRAY(uint32_t) colors;   //颜色
	DARRAY(struct vec2) texverts[16];//纹理坐标

	pthread_mutex_t effect_mutex;
    ///所有的着色器文件
	struct gs_effect *first_effect;

    ///每次绘制都会锁住 切引用计数+1 
	pthread_mutex_t mutex;
	volatile long ref;

	struct blend_state cur_blend_state;
	DARRAY(struct blend_state) blend_state_stack;

	bool linear_srgb;
};
