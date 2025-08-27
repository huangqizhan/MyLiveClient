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

#include <util/darray.h>
#include <util/threading.h>
#include <graphics/graphics.h>
#include <graphics/device-exports.h>
#include <graphics/matrix4.h>

#include <glad/glad.h>

#include "gl-helpers.h"

struct gl_platform;
struct gl_windowinfo;

/*
 COPY_TYPE_ARB: GLAD_GL_ARB_copy_image:
 这是一个 OpenGL 扩展名称,代表 "ARB Copy Image"。
 ARB 是 OpenGL Architecture Review Board 的缩写,它负责管理和维护 OpenGL 标准。
 这个扩展提供了一种高效的方式来复制图像数据,而无需使用 glCopyTexImage2D 或 glReadPixels 等传统方法。
 使用这个扩展,可以在不同的纹理对象之间直接进行快速复制,而无需经过 CPU 处理。
 这个扩展通常在需要高性能图像复制的场景中使用,例如在游戏引擎或图形处理应用程序中。
 GLAD_GL_NV_copy_image:

 COPY_TYPE_NV: 这是另一个 OpenGL 扩展名称,代表 "NV Copy Image"。
 NV 是 NVIDIA 公司的缩写,这个扩展是由 NVIDIA 开发的。
 这个扩展提供了与 GLAD_GL_ARB_copy_image 类似的功能,允许在不同的纹理对象之间进行高效的图像复制。
 与 ARB 扩展不同,这个扩展是由 NVIDIA 公司开发的,可能只在 NVIDIA 显卡上可用。
 如果硬件不支持 ARB 扩展,可以考虑使用 NV 扩展作为备用方案。
 
 COPY_TYPE_FBO_BLIT: 通过cpu 拷贝
 */
enum copy_type { COPY_TYPE_ARB, COPY_TYPE_NV, COPY_TYPE_FBO_BLIT };

static inline GLenum convert_gs_format(enum gs_color_format format)
{
	switch (format) {
	case GS_A8:
		return GL_RED;
	case GS_R8:
		return GL_RED;
	case GS_RGBA:
		return GL_RGBA;
	case GS_BGRX:
		return GL_BGRA;
	case GS_BGRA:
		return GL_BGRA;
	case GS_R10G10B10A2:
		return GL_RGBA;
	case GS_RGBA16:
		return GL_RGBA;
	case GS_R16:
		return GL_RED;
	case GS_RGBA16F:
		return GL_RGBA;
	case GS_RGBA32F:
		return GL_RGBA;
	case GS_RG16F:
		return GL_RG;
	case GS_RG32F:
		return GL_RG;
	case GS_R8G8:
		return GL_RG;
	case GS_R16F:
		return GL_RED;
	case GS_R32F:
		return GL_RED;
	case GS_DXT1:
		return GL_RGB;
	case GS_DXT3:
		return GL_RGBA;
	case GS_DXT5:
		return GL_RGBA;
	case GS_RGBA_UNORM:
		return GL_RGBA;
	case GS_BGRX_UNORM:
		return GL_BGRA;
	case GS_BGRA_UNORM:
		return GL_BGRA;
	case GS_RG16:
		return GL_RG;
	case GS_UNKNOWN:
		return 0;
	}

	return 0;
}

static inline GLenum convert_gs_internal_format(enum gs_color_format format)
{
	switch (format) {
	case GS_A8:
		return GL_R8; /* NOTE: use GL_TEXTURE_SWIZZLE_x */
	case GS_R8:
		return GL_R8;
	case GS_RGBA:
		return GL_SRGB8_ALPHA8;
	case GS_BGRX:
		return GL_SRGB8;
	case GS_BGRA:
		return GL_SRGB8_ALPHA8;
	case GS_R10G10B10A2:
		return GL_RGB10_A2;
	case GS_RGBA16:
		return GL_RGBA16;
	case GS_R16:
		return GL_R16;
	case GS_RGBA16F:
		return GL_RGBA16F;
	case GS_RGBA32F:
		return GL_RGBA32F;
	case GS_RG16F:
		return GL_RG16F;
	case GS_RG32F:
		return GL_RG32F;
	case GS_R8G8:
		return GL_RG8;
	case GS_R16F:
		return GL_R16F;
	case GS_R32F:
		return GL_R32F;
	case GS_DXT1:
		return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
	case GS_DXT3:
		return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
	case GS_DXT5:
		return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	case GS_RGBA_UNORM:
		return GL_RGBA;
	case GS_BGRX_UNORM:
		return GL_RGB;
	case GS_BGRA_UNORM:
		return GL_RGBA;
	case GS_RG16:
		return GL_RG16;
	case GS_UNKNOWN:
		return 0;
	}

	return 0;
}

static inline GLenum get_gl_format_type(enum gs_color_format format)
{
	switch (format) {
	case GS_A8:
		return GL_UNSIGNED_BYTE;
	case GS_R8:
		return GL_UNSIGNED_BYTE;
	case GS_RGBA:
		return GL_UNSIGNED_BYTE;
	case GS_BGRX:
		return GL_UNSIGNED_BYTE;
	case GS_BGRA:
		return GL_UNSIGNED_BYTE;
	case GS_R10G10B10A2:
		return GL_UNSIGNED_INT_2_10_10_10_REV;
	case GS_RGBA16:
		return GL_UNSIGNED_SHORT;
	case GS_R16:
		return GL_UNSIGNED_SHORT;
	case GS_RGBA16F:
		return GL_HALF_FLOAT;
	case GS_RGBA32F:
		return GL_FLOAT;
	case GS_RG16F:
		return GL_HALF_FLOAT;
	case GS_RG32F:
		return GL_FLOAT;
	case GS_R8G8:
		return GL_UNSIGNED_BYTE;
	case GS_R16F:
		return GL_HALF_FLOAT;
	case GS_R32F:
		return GL_FLOAT;
	case GS_DXT1:
		return GL_UNSIGNED_BYTE;
	case GS_DXT3:
		return GL_UNSIGNED_BYTE;
	case GS_DXT5:
		return GL_UNSIGNED_BYTE;
	case GS_RGBA_UNORM:
		return GL_UNSIGNED_BYTE;
	case GS_BGRX_UNORM:
		return GL_UNSIGNED_BYTE;
	case GS_BGRA_UNORM:
		return GL_UNSIGNED_BYTE;
	case GS_RG16:
		return GL_UNSIGNED_SHORT;
	case GS_UNKNOWN:
		return 0;
	}

	return GL_UNSIGNED_BYTE;
}
///模版类型转换
static inline GLenum convert_zstencil_format(enum gs_zstencil_format format)
{
	switch (format) {
	case GS_Z16:
		return GL_DEPTH_COMPONENT16;
	case GS_Z24_S8:
		return GL_DEPTH24_STENCIL8;
	case GS_Z32F:
		return GL_DEPTH_COMPONENT32F;
	case GS_Z32F_S8X24:
		return GL_DEPTH32F_STENCIL8;
	case GS_ZS_NONE:
		return 0;
	}

	return 0;
}
///深度类型转换
static inline GLenum convert_gs_depth_test(enum gs_depth_test test)
{
	switch (test) {
	case GS_NEVER:
		return GL_NEVER;
	case GS_LESS:
		return GL_LESS;
	case GS_LEQUAL:
		return GL_LEQUAL;
	case GS_EQUAL:
		return GL_EQUAL;
	case GS_GEQUAL:
		return GL_GEQUAL;
	case GS_GREATER:
		return GL_GREATER;
	case GS_NOTEQUAL:
		return GL_NOTEQUAL;
	case GS_ALWAYS:
		return GL_ALWAYS;
	}

	return GL_NEVER;
}

static inline GLenum convert_gs_stencil_op(enum gs_stencil_op_type op)
{
	switch (op) {
	case GS_KEEP:
		return GL_KEEP;
	case GS_ZERO:
		return GL_ZERO;
	case GS_REPLACE:
		return GL_REPLACE;
	case GS_INCR:
		return GL_INCR;
	case GS_DECR:
		return GL_DECR;
	case GS_INVERT:
		return GL_INVERT;
	}

	return GL_KEEP;
}

static inline GLenum convert_gs_stencil_side(enum gs_stencil_side side)
{
	switch (side) {
	case GS_STENCIL_FRONT:
		return GL_FRONT;
	case GS_STENCIL_BACK:
		return GL_BACK;
	case GS_STENCIL_BOTH:
		return GL_FRONT_AND_BACK;
	}

	return GL_FRONT;
}

static inline GLenum convert_gs_blend_type(enum gs_blend_type type)
{
	switch (type) {
	case GS_BLEND_ZERO:
		return GL_ZERO;
	case GS_BLEND_ONE:
		return GL_ONE;
	case GS_BLEND_SRCCOLOR:
		return GL_SRC_COLOR;
	case GS_BLEND_INVSRCCOLOR:
		return GL_ONE_MINUS_SRC_COLOR;
	case GS_BLEND_SRCALPHA:
		return GL_SRC_ALPHA;
	case GS_BLEND_INVSRCALPHA:
		return GL_ONE_MINUS_SRC_ALPHA;
	case GS_BLEND_DSTCOLOR:
		return GL_DST_COLOR;
	case GS_BLEND_INVDSTCOLOR:
		return GL_ONE_MINUS_DST_COLOR;
	case GS_BLEND_DSTALPHA:
		return GL_DST_ALPHA;
	case GS_BLEND_INVDSTALPHA:
		return GL_ONE_MINUS_DST_ALPHA;
	case GS_BLEND_SRCALPHASAT:
		return GL_SRC_ALPHA_SATURATE;
	}

	return GL_ONE;
}

static inline GLenum convert_gs_blend_op_type(enum gs_blend_op_type type)
{
	switch (type) {
	case GS_BLEND_OP_ADD:
		return GL_FUNC_ADD;
	case GS_BLEND_OP_SUBTRACT:
		return GL_FUNC_SUBTRACT;
	case GS_BLEND_OP_REVERSE_SUBTRACT:
		return GL_FUNC_REVERSE_SUBTRACT;
	case GS_BLEND_OP_MIN:
		return GL_MIN;
	case GS_BLEND_OP_MAX:
		return GL_MAX;
	}

	return GL_FUNC_ADD;
}

static inline GLenum convert_shader_type(enum gs_shader_type type)
{
	switch (type) {
	case GS_SHADER_VERTEX:
		return GL_VERTEX_SHADER;
	case GS_SHADER_PIXEL:
		return GL_FRAGMENT_SHADER;
	}

	return GL_VERTEX_SHADER;
}

static inline void convert_filter(enum gs_sample_filter filter,
				  GLint *min_filter, GLint *mag_filter)
{
	switch (filter) {
	case GS_FILTER_POINT:
		*min_filter = GL_NEAREST_MIPMAP_NEAREST;
		*mag_filter = GL_NEAREST;
		return;
	case GS_FILTER_LINEAR:
		*min_filter = GL_LINEAR_MIPMAP_LINEAR;
		*mag_filter = GL_LINEAR;
		return;
	case GS_FILTER_MIN_MAG_POINT_MIP_LINEAR:
		*min_filter = GL_NEAREST_MIPMAP_LINEAR;
		*mag_filter = GL_NEAREST;
		return;
	case GS_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT:
		*min_filter = GL_NEAREST_MIPMAP_NEAREST;
		*mag_filter = GL_LINEAR;
		return;
	case GS_FILTER_MIN_POINT_MAG_MIP_LINEAR:
		*min_filter = GL_NEAREST_MIPMAP_LINEAR;
		*mag_filter = GL_LINEAR;
		return;
	case GS_FILTER_MIN_LINEAR_MAG_MIP_POINT:
		*min_filter = GL_LINEAR_MIPMAP_NEAREST;
		*mag_filter = GL_NEAREST;
		return;
	case GS_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
		*min_filter = GL_LINEAR_MIPMAP_LINEAR;
		*mag_filter = GL_NEAREST;
		return;
	case GS_FILTER_MIN_MAG_LINEAR_MIP_POINT:
		*min_filter = GL_LINEAR_MIPMAP_NEAREST;
		*mag_filter = GL_LINEAR;
		return;
	case GS_FILTER_ANISOTROPIC:
		*min_filter = GL_LINEAR_MIPMAP_LINEAR;
		*mag_filter = GL_LINEAR;
		return;
	}

	*min_filter = GL_NEAREST_MIPMAP_NEAREST;
	*mag_filter = GL_NEAREST;
}
///寻址模式
static inline GLint convert_address_mode(enum gs_address_mode mode)
{
	switch (mode) {
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
	}

	return GL_REPEAT;
}

static inline GLenum convert_gs_topology(enum gs_draw_mode mode)
{
	switch (mode) {
	case GS_POINTS:
		return GL_POINTS;
	case GS_LINES:
		return GL_LINES;
	case GS_LINESTRIP:
		return GL_LINE_STRIP;
	case GS_TRIS:
		return GL_TRIANGLES;
	case GS_TRISTRIP:
		return GL_TRIANGLE_STRIP;
	}

	return GL_POINTS;
}

extern void convert_sampler_info(struct gs_sampler_state *sampler,
				 const struct gs_sampler_info *info);

/*
 uniform sampler2D  texture1
 sampler2D采样器的状态
 */
struct gs_sampler_state {
	gs_device_t *device;
	volatile long ref;
    /*
     当纹理被缩小时如何进行过滤。它可以是以下值之一:
     GL_NEAREST
     GL_LINEAR
     GL_NEAREST_MIPMAP_NEAREST
     GL_LINEAR_MIPMAP_NEAREST
     GL_NEAREST_MIPMAP_LINEAR
     GL_LINEAR_MIPMAP_LINEAR
     */
	GLint min_filter;
    /*
     当纹理被放大时如何进行过滤。它可以是 GL_NEAREST 或 GL_LINEAR。
     */
	GLint mag_filter;
    /*
     这些是纹理坐标的寻址模式,决定了当纹理坐标超出 [0, 1] 范围时如何处理。它们可以是以下值之一:
     GL_REPEAT
     GL_MIRRORED_REPEAT
     GL_CLAMP_TO_EDGE
     GL_CLAMP_TO_BORDER
     */
	GLint address_u;
	GLint address_v;
	GLint address_w;
    
    /*
     用于在纹理被斜视时提高图像质量
     */
	GLint max_anisotropy;
    /*
     寻址模式设置为 GL_CLAMP_TO_BORDER 时使用的边框颜色
     */
	struct vec4 border_color;
};

static inline void samplerstate_addref(gs_samplerstate_t *ss)
{
	os_atomic_inc_long(&ss->ref);
}

static inline void samplerstate_release(gs_samplerstate_t *ss)
{
	if (os_atomic_dec_long(&ss->ref) == 0)
		bfree(ss);
}

struct gs_timer {
	GLuint queries[2];
};

struct gs_shader_param {
	enum gs_shader_param_type type;

	char *name;
	gs_shader_t *shader;
	gs_samplerstate_t *next_sampler;
	GLint texture_id;
	size_t sampler_id;
	int array_count;

	struct gs_texture *texture;
	bool srgb;

	DARRAY(uint8_t) cur_value;
	DARRAY(uint8_t) def_value;
	bool changed;
};

enum attrib_type {
	ATTRIB_POSITION, // 顶点的位置属性，通常用于定义3D空间中的顶点坐标
	ATTRIB_NORMAL,   // 顶点的法线属性，通常用于光照计算。
	ATTRIB_TANGENT,  // 顶点的切线属性，通常用于法线贴图（normal mapping）等高级渲染技术。
	ATTRIB_COLOR,    //顶点的颜色属性，通常用于给顶点上色
	ATTRIB_TEXCOORD, //顶点的纹理坐标属性，用于纹理映射
	ATTRIB_TARGET    //
};
///顶点属性  VAO
struct shader_attrib {
	char *name;
	size_t index;
	enum attrib_type type;
};
///着色器 
struct gs_shader {
	gs_device_t *device;
	enum gs_shader_type type;
	GLuint obj;
    
    ///张相机矩阵 它将 3D 世界坐标系转换为观察者的视角坐标系
	struct gs_shader_param *viewproj;
    ///世界矩阵 将2D坐标转换为3D坐标 (世界坐标)
	struct gs_shader_param *world;
    
    //VAO
	DARRAY(struct shader_attrib) attribs;
	DARRAY(struct gs_shader_param) params;
	DARRAY(gs_samplerstate_t *) samplers;
};
///着色器程序的参数
struct program_param {
	GLint obj;
	struct gs_shader_param *param;
};
/*
 可执行的着色器程序,由一个或多个着色器组成
 glCreateProgram();
 */
struct gs_program {
	gs_device_t *device;
	GLuint obj;
	struct gs_shader *vertex_shader;
	struct gs_shader *pixel_shader;

	DARRAY(struct program_param) params;
	DARRAY(GLint) attribs;

	struct gs_program **prev_next;
	struct gs_program *next;
};

extern struct gs_program *gs_program_create(struct gs_device *device);
extern void gs_program_destroy(struct gs_program *program);
extern void program_update_params(struct gs_program *shader);

///OpenGL顶点相关的缓冲
struct gs_vertex_buffer {
    //VAO
	GLuint vao;
    
    //VBO
	GLuint vertex_buffer;           //顶点位置
	GLuint normal_buffer;           //顶点法线
	GLuint tangent_buffer;          //顶点切线
	GLuint color_buffer;            //定点颜色
	
    DARRAY(GLuint) uv_buffers;      //顶点纹理坐标
	DARRAY(size_t) uv_sizes;       //顶点纹理坐标的维度

	gs_device_t *device;
    
    //坐标的数量
	size_t num;
	bool dynamic;
    ///UV buffer 
	struct gs_vb_data *data;
};

extern bool load_vb_buffers(struct gs_program *program,
			    struct gs_vertex_buffer *vb,
			    struct gs_index_buffer *ib);

struct gs_index_buffer {
	GLuint buffer;
	enum gs_index_type type;
	GLuint gl_type;

	gs_device_t *device;
	void *data;
	size_t num;
	size_t width;
	size_t size;
	bool dynamic;
};

struct gs_texture {
	gs_device_t *device;
	enum gs_texture_type type;
	enum gs_color_format format;
	GLenum gl_format;
    //GL_TEXTURE_2D
	GLenum gl_target;
	GLenum gl_internal_format;
	GLenum gl_type;
	GLuint texture;
	uint32_t levels;
	bool is_dynamic;
	bool is_render_target;
    ///虚拟纹理
	bool is_dummy;
	bool gen_mipmaps;
    ///当前的采样器
	gs_samplerstate_t *cur_sampler;
	struct fbo_info *fbo;
};

struct gs_texture_2d {
	struct gs_texture base;

	uint32_t width;
	uint32_t height;
	bool gen_mipmaps;
	GLuint unpack_buffer;
};

struct gs_texture_3d {
	struct gs_texture base;

	uint32_t width;
	uint32_t height;
	uint32_t depth;
	bool gen_mipmaps;
	GLuint unpack_buffer;
};

struct gs_texture_cube {
	struct gs_texture base;

	uint32_t size;
};
///暂存帧缓冲的表面数据  它在需要频繁读取像素数据的场景中非常有用。
struct gs_stage_surface {
	gs_device_t *device;

	enum gs_color_format format;
	uint32_t width;
	uint32_t height;

	uint32_t bytes_per_pixel;
	GLenum gl_format;
	GLint gl_internal_format;
	GLenum gl_type;
	GLuint pack_buffer;
};
///深度模版缓冲(跟帧缓冲的渲染缓冲相关)
struct gs_zstencil_buffer {
	gs_device_t *device;
	GLuint buffer; ///渲染缓冲
	GLuint attachment; ///渲染缓冲的附加类型
	GLenum format;     ///深度类型 
};
///一个display 持有一个swap 一个swap持有一个OpenGLContext 
struct gs_swap_chain {
	gs_device_t *device;
	struct gl_windowinfo *wi;
	struct gs_init_data info;
};
/*
 vao（vertice attribute objec）顶点数据属性
 vbo（vertice buffer objec）  顶点缓冲 GL_ARRAY_BUFFER
 ebo 顶点索引数据对象 GL_ELEMENT_ARRAY_BUFFER
 
 fbo (frame buffer oject)
 
 颜色缓冲、深度缓冲以及模板缓冲，这些缓冲结合起来叫做帧缓冲(Framebuffer)，它被储存在GPU内存中的某处。OpenGL允许我们定义我们自己的帧缓冲，也就是说我们能够定义我们自己的颜色缓冲，甚至是深度缓冲和模板缓冲。
 
 // 创建fbo
 unsigned int fbo;
 glGenFramebuffers(1, &fbo);
 
 
   gl_bind_framebuffer(
 //绑定fbo
 glBindFramebuffer(GL_FRAMEBUFFER, fbo);
 GL_READ_FRAMEBUFFER: 将一个帧缓冲分别绑定到读取目标  绑定到GL_READ_FRAMEBUFFER的帧缓冲将会使用在所有像是glReadPixels的读取操作中，
 GL_DRAW_FRAMEBUFFER: 将一个帧缓冲分别绑定到写入目标  绑定到GL_DRAW_FRAMEBUFFER的帧缓冲将会被用作渲染、清除等写入操作的目标。
 GL_FRAMEBUFFER: 所有的读取和写入帧缓冲的操作 使用GL_FRAMEBUFFER，绑定到读写两个上
 
 一个完整的帧缓冲需要满足以下的条件：
    附加至少一个缓冲（颜色、深度或模板缓冲）。
    至少有一个颜色附件(Attachment)。
    所有的附件都必须是完整的（保留了内存）。
    每个缓冲都应该有相同的样本数(sample)。
 
 ///附加纹理附件
 {
     将创建好的纹理附加到帧缓冲上:
     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
     
     glFrameBufferTexture2D有以下的参数：

     target：帧缓冲的目标（绘制、读取或者两者皆有）
     attachment：我们想要附加的附件类型。当前我们正在附加一个颜色附件。注意最后的0意味着我们可以附加多个颜色附件。我们将在之后的教程中提到。
     textarget：你希望附加的纹理类型
     texture：要附加的纹理本身
     level：多级渐远纹理的级别。我们将它保留为0。
     
     
     将深度缓冲附加到帧缓冲上:
     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT, texture, 0);
     
     
     将模版缓冲附加到帧缓冲上:
     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_STENCIL_INDEX, texture, 0);
     
     也可以将深度缓冲和模板缓冲附加为一个单独的纹理。纹理的每32位数值将包含24位的深度信息和8位的模板信息。要将深度和模板缓冲附加为一个纹理的话，我们使用GL_DEPTH_STENCIL_ATTACHMENT类型，并配置纹理的格式，让它包含合并的深度和模板值。将一个深度和模板缓冲附加为一个纹理到帧缓冲的例子可以在下面找到：

     glTexImage2D(
       GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 800, 600, 0,
       GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL
     );
      
     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture, 0);

 }
 //附加渲染缓冲对象附件
  {
    创建一个渲染缓冲对象：
     unsigned int rbo;
     glGenRenderbuffers(1, &rbo);
 
    绑定这个渲染缓冲对象：
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
 
    创建一个深度和模板渲染缓冲对象：
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 800, 600);
    
    附加这个渲染缓冲对象：
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
  }
 */
struct fbo_info {
	GLuint fbo;
	uint32_t width;
	uint32_t height;
	enum gs_color_format format;

	gs_texture_t *cur_render_target;
	int cur_render_side;
	gs_zstencil_t *cur_zstencil_buffer;
};

static inline void fbo_info_destroy(struct fbo_info *fbo)
{
	if (fbo) {
		glDeleteFramebuffers(1, &fbo->fbo);
		gl_success("glDeleteFramebuffers");

		bfree(fbo);
	}
}
/*
 此对象全局只有一个
 当前设备会关联一个当前平台总的OpenGLContext
 总的OpenGLContext下会关联一个或多个子OpenGLContext
 一个子OpenGLContext对应一个具体的widget 
 
 
 先变换(平移、放缩、旋转)  ===》 viewCamera ===》 perspective ===》draw 
 
 
 */
struct gs_device {
    //当前平台总的OpenGLContext
	struct gl_platform *plat;
	enum copy_type copy_type;

	GLuint empty_vao;
	gs_samplerstate_t *raw_load_sampler;
    //当前渲染结果的存储对象 
	gs_texture_t *cur_render_target;
	gs_zstencil_t *cur_zstencil_buffer;
	int cur_render_side;
    
	gs_texture_t *cur_textures[GS_MAX_TEXTURES];
	gs_samplerstate_t *cur_samplers[GS_MAX_TEXTURES];
    
    ///vbo (vetices buffer object)顶点缓冲区
	gs_vertbuffer_t *cur_vertex_buffer;
    ///ebo () 顶点索引缓冲区
	gs_indexbuffer_t *cur_index_buffer;
    ///当前的顶点着色器 (effect->technique->pass->vertex_shader)
	gs_shader_t *cur_vertex_shader;
    ///片段着色器 (effect->technique->pass->piel_shader)
	gs_shader_t *cur_pixel_shader;
    ///每一个display 会有一个gs_swapchain_t device会切换不同的swap 
	gs_swapchain_t *cur_swap;
	struct gs_program *cur_program;
	enum gs_color_space cur_color_space;

	struct gs_program *first_program;

	enum gs_cull_mode cur_cull_mode;
	struct gs_rect cur_viewport;

    //透视
	struct matrix4 cur_proj;
	struct matrix4 cur_view;
	struct matrix4 cur_viewproj;
    ///透视矩阵栈
	DARRAY(struct matrix4) proj_stack;
    //device对应的当前fbo  （frame buffer object） 帧缓冲区
	struct fbo_info *cur_fbo;
};

extern struct fbo_info *get_fbo(gs_texture_t *tex, uint32_t width,
				uint32_t height);

extern void gl_update(gs_device_t *device);
extern void gl_clear_context(gs_device_t *device);

extern struct gl_platform *gl_platform_create(gs_device_t *device,
					      uint32_t adapter);
extern void gl_platform_destroy(struct gl_platform *platform);

extern bool gl_platform_init_swapchain(struct gs_swap_chain *swap);
extern void gl_platform_cleanup_swapchain(struct gs_swap_chain *swap);

extern struct gl_windowinfo *
gl_windowinfo_create(const struct gs_init_data *info);
extern void gl_windowinfo_destroy(struct gl_windowinfo *wi);

extern void gl_getclientsize(const struct gs_swap_chain *swap, uint32_t *width,
			     uint32_t *height);
