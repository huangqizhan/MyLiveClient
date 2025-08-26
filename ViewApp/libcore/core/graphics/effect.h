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

#include "effect-parser.h"
#include "graphics.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Effects introduce a means of bundling together shader text into one
 * file with shared functions and parameters.  This is done because often
 * shaders must be duplicated when you need to alter minor aspects of the code
 * that cannot be done via constants.  Effects allow developers to easily
 * switch shaders and set constants that can be used between shaders.
 *
 * Effects are built via the effect parser, and shaders are automatically
 * generated for each technique's pass.
 */

/* ------------------------------------------------------------------------- */

enum effect_section {
	EFFECT_PARAM,
	EFFECT_TECHNIQUE,
	EFFECT_SAMPLER,
	EFFECT_PASS,
	EFFECT_ANNOTATION
};

/* ------------------------------------------------------------------------- */
///effect级别的参数
struct gs_effect_param {
	char *name;
	enum effect_section section;

	enum gs_shader_param_type type;

	bool changed;   //参数的值是否被改变
	DARRAY(uint8_t) cur_val;
	DARRAY(uint8_t) default_val;

	gs_effect_t *effect;
	gs_samplerstate_t *next_sampler;

	/*char *full_name;  注释
	float scroller_min, scroller_max, scroller_inc, scroller_mul;*/
	DARRAY(struct gs_effect_param) annotations;
};

static inline void effect_param_init(struct gs_effect_param *param)
{
	memset(param, 0, sizeof(struct gs_effect_param));
	da_init(param->annotations);
}

static inline void effect_param_free(struct gs_effect_param *param)
{
	bfree(param->name);
	//bfree(param->full_name);
	da_free(param->cur_val);
	da_free(param->default_val);

	size_t i;
	for (i = 0; i < param->annotations.num; i++)
		effect_param_free(param->annotations.array + i);

	da_free(param->annotations);
}

EXPORT void effect_param_parse_property(gs_eparam_t *param,
					const char *property);

/* ------------------------------------------------------------------------- */
// pass所关联的参数
struct pass_shaderparam {
    //
	struct gs_effect_param *eparam;
	gs_sparam_t *sparam;
};
/*
 technique DrawPQ
 {
     pass
     {
         vertex_shader = VSDefault(vert_in);
         pixel_shader  = PSDrawPQ(vert_in);
     }
 }
 一个pass会有一个顶点着色器和一个片段着色器
 */
struct gs_effect_pass {
	char *name;
	enum effect_section section;

	gs_shader_t *vertshader;
	gs_shader_t *pixelshader;
	DARRAY(struct pass_shaderparam) vertshader_params;
	DARRAY(struct pass_shaderparam) pixelshader_params;
};

static inline void effect_pass_init(struct gs_effect_pass *pass)
{
	memset(pass, 0, sizeof(struct gs_effect_pass));
}

static inline void effect_pass_free(struct gs_effect_pass *pass)
{
	bfree(pass->name);
	da_free(pass->vertshader_params);
	da_free(pass->pixelshader_params);

	gs_shader_destroy(pass->vertshader);
	gs_shader_destroy(pass->pixelshader);
}

/* ------------------------------------------------------------------------- */
/*
 technique DrawPQ
 {
     pass
     {
         vertex_shader = VSDefault(vert_in);
         pixel_shader  = PSDrawPQ(vert_in);
     }
 }
 一个technique会有一个或多个pass
 
 name: DrawPQ
 
 
 */
struct gs_effect_technique {
	char *name;
	enum effect_section section;
	struct gs_effect *effect;
    ///多个着色器
	DARRAY(struct gs_effect_pass) passes;
};

static inline void effect_technique_init(struct gs_effect_technique *t)
{
	memset(t, 0, sizeof(struct gs_effect_technique));
}

static inline void effect_technique_free(struct gs_effect_technique *t)
{
	size_t i;
	for (i = 0; i < t->passes.num; i++)
		effect_pass_free(t->passes.array + i);

	da_free(t->passes);
	bfree(t->name);
}

/* ------------------------------------------------------------------------- */
/*
 
 uniform float4x4 ViewProj;
 uniform texture2d image;
 uniform float multiplier;

 sampler_state def_sampler {
     Filter   = Linear;
     AddressU = Clamp;
     AddressV = Clamp;
 };

 struct VertInOut {
     float4 pos : POSITION;
     float2 uv  : TEXCOORD0;
 };

 VertInOut VSDefault(VertInOut vert_in)
 {
     VertInOut vert_out;
     vert_out.pos = mul(float4(vert_in.pos.xyz, 1.0), ViewProj);
     vert_out.uv  = vert_in.uv;
     return vert_out;
 }

 float4 PSDrawBare(VertInOut vert_in) : TARGET
 {
     return image.Sample(def_sampler, vert_in.uv);
 }

 float4 PSDrawAlphaDivide(VertInOut vert_in) : TARGET
 {
     float4 rgba = image.Sample(def_sampler, vert_in.uv);
     rgba.rgb *= max(1. / rgba.a, 0.);
     return rgba;
 }

 float4 PSDrawNonlinearAlpha(VertInOut vert_in) : TARGET
 {
     float4 rgba = image.Sample(def_sampler, vert_in.uv);
     rgba.rgb = srgb_linear_to_nonlinear(rgba.rgb);
     rgba.rgb *= rgba.a;
     rgba.rgb = srgb_nonlinear_to_linear(rgba.rgb);
     return rgba;
 }

 float4 PSDrawNonlinearAlphaMultiply(VertInOut vert_in) : TARGET
 {
     float4 rgba = image.Sample(def_sampler, vert_in.uv);
     rgba.rgb = srgb_linear_to_nonlinear(rgba.rgb);
     rgba.rgb *= rgba.a;
     rgba.rgb = srgb_nonlinear_to_linear(rgba.rgb);
     rgba.rgb *= multiplier;
     return rgba;
 }

 float4 PSDrawSrgbDecompress(VertInOut vert_in) : TARGET
 {
     float4 rgba = image.Sample(def_sampler, vert_in.uv);
     rgba.rgb = srgb_nonlinear_to_linear(rgba.rgb);
     return rgba;
 }

 float4 PSDrawSrgbDecompressMultiply(VertInOut vert_in) : TARGET
 {
     float4 rgba = image.Sample(def_sampler, vert_in.uv);
     rgba.rgb = srgb_nonlinear_to_linear(rgba.rgb);
     rgba.rgb *= multiplier;
     return rgba;
 }

 float4 PSDrawMultiply(VertInOut vert_in) : TARGET
 {
     float4 rgba = image.Sample(def_sampler, vert_in.uv);
     rgba.rgb *= multiplier;
     return rgba;
 }

 float4 PSDrawTonemap(VertInOut vert_in) : TARGET
 {
     float4 rgba = image.Sample(def_sampler, vert_in.uv);
     rgba.rgb = rec709_to_rec2020(rgba.rgb);
     rgba.rgb = reinhard(rgba.rgb);
     rgba.rgb = rec2020_to_rec709(rgba.rgb);
     return rgba;
 }

 float4 PSDrawMultiplyTonemap(VertInOut vert_in) : TARGET
 {
     float4 rgba = image.Sample(def_sampler, vert_in.uv);
     rgba.rgb *= multiplier;
     rgba.rgb = rec709_to_rec2020(rgba.rgb);
     rgba.rgb = reinhard(rgba.rgb);
     rgba.rgb = rec2020_to_rec709(rgba.rgb);
     return rgba;
 }

 float4 PSDrawPQ(VertInOut vert_in) : TARGET
 {
     float4 rgba = image.Sample(def_sampler, vert_in.uv);
     rgba.rgb = st2084_to_linear(rgba.rgb) * multiplier;
     rgba.rgb = rec2020_to_rec709(rgba.rgb);
     return rgba;
 }

 float4 PSDrawTonemapPQ(VertInOut vert_in) : TARGET
 {
     float4 rgba = image.Sample(def_sampler, vert_in.uv);
     rgba.rgb = st2084_to_linear(rgba.rgb) * multiplier;
     rgba.rgb = reinhard(rgba.rgb);
     rgba.rgb = rec2020_to_rec709(rgba.rgb);
     return rgba;
 }

 technique Draw
 {
     pass
     {
         vertex_shader = VSDefault(vert_in);
         pixel_shader  = PSDrawBare(vert_in);
     }
 }

 technique DrawAlphaDivide
 {
     pass
     {
         vertex_shader = VSDefault(vert_in);
         pixel_shader  = PSDrawAlphaDivide(vert_in);
     }
 }

 technique DrawNonlinearAlpha
 {
     pass
     {
         vertex_shader = VSDefault(vert_in);
         pixel_shader  = PSDrawNonlinearAlpha(vert_in);
     }
 }

 technique DrawNonlinearAlphaMultiply
 {
     pass
     {
         vertex_shader = VSDefault(vert_in);
         pixel_shader  = PSDrawNonlinearAlphaMultiply(vert_in);
     }
 }

 technique DrawSrgbDecompress
 {
     pass
     {
         vertex_shader = VSDefault(vert_in);
         pixel_shader  = PSDrawSrgbDecompress(vert_in);
     }
 }

 technique DrawSrgbDecompressMultiply
 {
     pass
     {
         vertex_shader = VSDefault(vert_in);
         pixel_shader  = PSDrawSrgbDecompressMultiply(vert_in);
     }
 }

 technique DrawMultiply
 {
     pass
     {
         vertex_shader = VSDefault(vert_in);
         pixel_shader  = PSDrawMultiply(vert_in);
     }
 }

 technique DrawTonemap
 {
     pass
     {
         vertex_shader = VSDefault(vert_in);
         pixel_shader  = PSDrawTonemap(vert_in);
     }
 }

 technique DrawMultiplyTonemap
 {
     pass
     {
         vertex_shader = VSDefault(vert_in);
         pixel_shader  = PSDrawMultiplyTonemap(vert_in);
     }
 }

 technique DrawPQ
 {
     pass
     {
         vertex_shader = VSDefault(vert_in);
         pixel_shader  = PSDrawPQ(vert_in);
     }
 }

 technique DrawTonemapPQ
 {
     pass
     {
         vertex_shader = VSDefault(vert_in);
         pixel_shader  = PSDrawTonemapPQ(vert_in);
     }
 }

 EFFECT_PARAM,
 EFFECT_TECHNIQUE,
 EFFECT_SAMPLER,
 EFFECT_PASS,
 EFFECT_ANNOTATION
 以上是一个.effect文件  有以下部分
 1:全局变量
 2:申明的类型
 2:着色器函数
 3:technique
 其中每一部分的任意组合会生成一个着色器
 每一个pass会生成一个顶点着色器 跟pixel着色器
 
 
 
 顶点着色器:
 uniform float4x4 ViewProj;
 
 struct VertInOut {
     float4 pos : POSITION;
     float2 uv : TEXCOORD0;
 };
 VertInOut VSDefault(VertInOut vert_in){
     tVertInOut vert_out;
     tvert_out.pos = mul(float4(vert_in.pos.xyz, 1.0),ViewProj);
     tvert_out.uv  = vert_in.uv;
     treturn vert_out;
 }
 VertInOut main(VertInOut vert_in){
    return VSDefault(vert_in);
 }
 
 
 像素着色器:
 uniform texture2d image;
 sampler_state def_sampler {
     Filter = Linear;
     AddressU = Clamp;
     AddressV = Clamp;
 }
 
 struct VertInOut {
     float4 pos : POSITION;
     float2 uv : TEXCOORD0;
 }
 float4 PSDrawBare(VertInOut vert_in){
    return image.Sample(def_sampler, vert_in.uv);
 }
 float4 main(VertInOut vert_in) : TARGET {
    return PSDrawBare(vert_in);
 }
 TARGET 表示此函数的输出是渲染目标（即屏幕上的像素颜色  片段着色器）
 
 effect中是一种跨平台的写法 需要兼容不同平台OpenGL/d3d11
 因此有些函数和变量类型在相应的平台要做替换
 
 各种着色器的处理都在这里
 
 */
struct gs_effect {
	bool processing;
	bool cached;
	char *effect_path, *effect_dir;
    /*
     uniform float4x4 ViewProj;
     uniform texture2d image;
     uniform float multiplier;
     */
	DARRAY(struct gs_effect_param) params;
    ///着色器程序
	DARRAY(struct gs_effect_technique) techniques;
    ///当前正在使用的technique
	struct gs_effect_technique *cur_technique;
    ///当前正在使用technique下的pass
	struct gs_effect_pass *cur_pass;

	gs_eparam_t *view_proj, *world, *scale;
	graphics_t *graphics;

	struct gs_effect *next;

	size_t loop_pass;
	bool looping;
};

static inline void effect_init(gs_effect_t *effect)
{
	memset(effect, 0, sizeof(struct gs_effect));
}

static inline void effect_free(gs_effect_t *effect)
{
	size_t i;
	for (i = 0; i < effect->params.num; i++)
		effect_param_free(effect->params.array + i);
	for (i = 0; i < effect->techniques.num; i++)
		effect_technique_free(effect->techniques.array + i);

	da_free(effect->params);
	da_free(effect->techniques);

	bfree(effect->effect_path);
	bfree(effect->effect_dir);
	effect->effect_path = NULL;
	effect->effect_dir = NULL;
}

EXPORT void effect_upload_params(gs_effect_t *effect, bool changed_only);
EXPORT void effect_upload_shader_params(gs_effect_t *effect,
					gs_shader_t *shader,
					struct darray *pass_params,
					bool changed_only);

#ifdef __cplusplus
}
#endif
