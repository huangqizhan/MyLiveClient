/******************************************************************************
    Copyright (C) 2013 by Ruwen Hahn <palana@stunned.de>

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

#include "gl-subsystem.h"
#include <OpenGL/OpenGL.h>

#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>
///每一个widget对应一个的子OpenGLContext和帧缓冲 (帧缓冲会默认绑定一个纹理)
///NSOpenGLView 都有一个对应的 NSOpenGLContext 实例。这个上下文维护了该视图的 OpenGL 状态
struct gl_windowinfo {
	NSView *view;
	NSOpenGLContext *context;
	gs_texture_t *texture;
	GLuint fbo;
};
///总的OpenGLContext
struct gl_platform {
	NSOpenGLContext *context;
};

static NSOpenGLContext *gl_context_create(NSOpenGLContext *share)
{
	NSOpenGLPixelFormatAttribute attributes[] = {
		NSOpenGLPFADoubleBuffer, NSOpenGLPFAOpenGLProfile,
		NSOpenGLProfileVersion3_2Core, 0};
   //如果有多个 NSOpenGLView 实例需要访问相同的 OpenGL 资源,那么可以让它们共享一个 NSOpenGLContext 对象,从而避免重复加载和管理这些资源。  如果 shareContext 参数传递为 nil,则新创建的 NSOpenGLContext 对象不会与任何其他上下文共享资源和状态。
	NSOpenGLPixelFormat *pf;
	pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
	if (!pf) {
		blog(LOG_ERROR, "Failed to create pixel format");
		return NULL;
	}

	NSOpenGLContext *context;
	context = [[NSOpenGLContext alloc] initWithFormat:pf
					     shareContext:share];
	[pf release];
	if (!context) {
		blog(LOG_ERROR, "Failed to create context");
		return NULL;
	}

	[context clearDrawable];

	return context;
}

struct gl_platform *gl_platform_create(gs_device_t *device __unused,
				       uint32_t adapter __unused)
{
	NSOpenGLContext *context = gl_context_create(nil);
	if (!context) {
		blog(LOG_ERROR, "gl_context_create failed");
		return NULL;
	}

	[context makeCurrentContext];
	GLint interval = 0;
	[context setValues:&interval
		forParameter:NSOpenGLContextParameterSwapInterval];
	const bool success = gladLoadGL() != 0;

	if (!success) {
		blog(LOG_ERROR, "gladLoadGL failed");
		[context release];
		return NULL;
	}

	struct gl_platform *plat = bzalloc(sizeof(struct gl_platform));
	plat->context = context;
	return plat;
}

void gl_platform_destroy(struct gl_platform *platform)
{
	if (!platform)
		return;

	[platform->context release];
	platform->context = nil;

	bfree(platform);
}

bool gl_platform_init_swapchain(struct gs_swap_chain *swap)
{
	NSOpenGLContext *parent = swap->device->plat->context;
	NSOpenGLContext *context = gl_context_create(parent);
	bool success = context != nil;
	if (success) {
		CGLContextObj parent_obj = [parent CGLContextObj];
		CGLLockContext(parent_obj);

		[parent makeCurrentContext];
		struct gs_init_data *init_data = &swap->info;
		swap->wi->texture = device_texture_create(
			swap->device, init_data->cx, init_data->cy,
			init_data->format, 1, NULL, GS_RENDER_TARGET);
		glFlush();
		[NSOpenGLContext clearCurrentContext];

		CGLContextObj context_obj = [context CGLContextObj];
		CGLLockContext(context_obj);

		[context makeCurrentContext];

#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		[context setView:swap->wi->view];
#pragma clang diagnostic pop
		GLint interval = 0;
		[context setValues:&interval
			forParameter:NSOpenGLContextParameterSwapInterval];
		gl_gen_framebuffers(1, &swap->wi->fbo);
		gl_bind_framebuffer(GL_FRAMEBUFFER, swap->wi->fbo);
        //将创建好的纹理附加到帧缓冲上
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				       GL_TEXTURE_2D,
				       swap->wi->texture->texture, 0);
		gl_success("glFrameBufferTexture2D");
		glFlush();
		[NSOpenGLContext clearCurrentContext];

		CGLUnlockContext(context_obj);

		CGLUnlockContext(parent_obj);

		swap->wi->context = context;
	}

	return success;
}

void gl_platform_cleanup_swapchain(struct gs_swap_chain *swap)
{
	NSOpenGLContext *parent = swap->device->plat->context;
	CGLContextObj parent_obj = [parent CGLContextObj];
	CGLLockContext(parent_obj);

	NSOpenGLContext *context = swap->wi->context;
	CGLContextObj context_obj = [context CGLContextObj];
	CGLLockContext(context_obj);

	[context makeCurrentContext];
	gl_delete_framebuffers(1, &swap->wi->fbo);
	glFlush();
	[NSOpenGLContext clearCurrentContext];

	CGLUnlockContext(context_obj);

	[parent makeCurrentContext];
	gs_texture_destroy(swap->wi->texture);
	glFlush();
	[NSOpenGLContext clearCurrentContext];
	swap->wi->context = nil;

	CGLUnlockContext(parent_obj);
}

struct gl_windowinfo *gl_windowinfo_create(const struct gs_init_data *info)
{
	if (!info)
		return NULL;

	if (!info->window.view)
		return NULL;

	struct gl_windowinfo *wi = bzalloc(sizeof(struct gl_windowinfo));

	wi->view = info->window.view;
	wi->view.window.colorSpace = NSColorSpace.sRGBColorSpace;
#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	wi->view.wantsBestResolutionOpenGLSurface = YES;
#pragma clang diagnostic pop

	return wi;
}

void gl_windowinfo_destroy(struct gl_windowinfo *wi)
{
	if (!wi)
		return;

	wi->view = nil;
	bfree(wi);
}

void gl_update(gs_device_t *device)
{
	gs_swapchain_t *swap = device->cur_swap;
	NSOpenGLContext *parent = device->plat->context;
	NSOpenGLContext *context = swap->wi->context;
	dispatch_async(dispatch_get_main_queue(), ^() {
		if (!swap || !swap->wi) {
			return;
		}

		CGLContextObj parent_obj = [parent CGLContextObj];
		CGLLockContext(parent_obj);

		CGLContextObj context_obj = [context CGLContextObj];
		CGLLockContext(context_obj);

		[context makeCurrentContext];
		[context update];
		struct gs_init_data *info = &swap->info;
		gs_texture_t *previous = swap->wi->texture;
        ///此处重新创建纹理
		swap->wi->texture = device_texture_create(device, info->cx,
							  info->cy,
							  info->format, 1, NULL,
							  GS_RENDER_TARGET);
        ///绑定fbo到GL_FRAMEBUFFER
		gl_bind_framebuffer(GL_FRAMEBUFFER, swap->wi->fbo);
        /*
         将渲染的结果写入到纹理中,而不是直接渲染到屏幕上
         在进行离屏渲染或渲染到纹理的场景中,需要先创建一个帧缓冲区对象,然后使用 glFramebufferTexture2D 将纹理附加到帧缓冲区上。
         这样可以将渲染结果写入到纹理中,供后续使用,如进行后期处理、保存图像等。
         */
        //将创建好的纹理附加到帧缓冲上
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				       GL_TEXTURE_2D,
				       swap->wi->texture->texture, 0);
		gl_success("glFrameBufferTexture2D");
		gs_texture_destroy(previous);
        //制将命令队列中的所有 OpenGL 命令立即提交给图形硬件执行
		glFlush();
		[NSOpenGLContext clearCurrentContext];

		CGLUnlockContext(context_obj);

		CGLUnlockContext(parent_obj);
	});
}

void gl_clear_context(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	[NSOpenGLContext clearCurrentContext];
}

void device_enter_context(gs_device_t *device)
{
	CGLLockContext([device->plat->context CGLContextObj]);

	[device->plat->context makeCurrentContext];
}

void device_leave_context(gs_device_t *device)
{
	glFlush();
	[NSOpenGLContext clearCurrentContext];
	device->cur_vertex_buffer = NULL;
	device->cur_index_buffer = NULL;
	device->cur_render_target = NULL;
	device->cur_zstencil_buffer = NULL;
	device->cur_swap = NULL;
	device->cur_fbo = NULL;

	CGLUnlockContext([device->plat->context CGLContextObj]);
}

void *device_get_device_obj(gs_device_t *device)
{
	return device->plat->context;
}

void device_load_swapchain(gs_device_t *device, gs_swapchain_t *swap)
{
	if (device->cur_swap == swap)
		return;

	device->cur_swap = swap;
	if (swap) {
		device_set_render_target(device, swap->wi->texture, NULL);
	}
}

bool device_is_present_ready(gs_device_t *device __unused)
{
	return true;
}
/*
 先将渲染结果输出到一个帧缓冲区(如 gBuffer)中,
 然后从这个帧缓冲区中读取数据进行后续的处理,
 最后将处理后的结果渲染到屏幕上。
 */
void device_present(gs_device_t *device)
{
    //确定之前的命令都被执行
	glFlush();
    //NSOpenGLView 都有一个对应的 NSOpenGLContext 实例。这个上下文维护了该视图的 OpenGL 状态
    //当需要在不同的 OpenGL 上下文之间切换时,就需要先调用 [NSOpenGLContext clearCurrentContext];
	[NSOpenGLContext clearCurrentContext];

	CGLLockContext([device->cur_swap->wi->context CGLContextObj]);

	[device->cur_swap->wi->context makeCurrentContext];
    //把GL_READ_FRAMEBUFFER绑定到自定义 device->cur_swap->wi->fbo)的帧缓冲
	gl_bind_framebuffer(GL_READ_FRAMEBUFFER, device->cur_swap->wi->fbo);
    //在OpenGL中,默认的帧缓冲区(framebuffer)是ID为0的帧缓冲区,它代表屏幕缓冲区。
	gl_bind_framebuffer(GL_DRAW_FRAMEBUFFER, 0);
	const uint32_t width = device->cur_swap->info.cx;
	const uint32_t height = device->cur_swap->info.cy;
    //OpenGL使用的是右手坐标系,Y轴是从下到上的。而屏幕坐标系通常使用左手坐标系,Y轴是从上到下的
    //把GL_READ_FRAMEBUFFER拷贝到GL_DRAW_FRAMEBUFFER（即屏幕上--当前画布上）
	glBlitFramebuffer(0, 0, width, height, 0, height, width, 0,
			  GL_COLOR_BUFFER_BIT, GL_NEAREST);
	[device->cur_swap->wi->context flushBuffer];
	glFlush();
	[NSOpenGLContext clearCurrentContext];

	CGLUnlockContext([device->cur_swap->wi->context CGLContextObj]);
    ///切换到总的NSOpenGLContext
	[device->plat->context makeCurrentContext];
}

bool device_is_monitor_hdr(gs_device_t *device __unused, void *monitor __unused)
{
	return false;
}

void gl_getclientsize(const struct gs_swap_chain *swap, uint32_t *width,
		      uint32_t *height)
{
	if (width)
		*width = swap->info.cx;
	if (height)
		*height = swap->info.cy;
}

gs_texture_t *device_texture_create_from_iosurface(gs_device_t *device,
						   void *iosurf)
{
	IOSurfaceRef ref = (IOSurfaceRef)iosurf;
	struct gs_texture_2d *tex = bzalloc(sizeof(struct gs_texture_2d));

	OSType pf = IOSurfaceGetPixelFormat(ref);

	FourCharCode l10r_code = 0;//
	l10r_code = ('l' << 24) | ('1' << 16) | ('0' << 8) | 'r';

	FourCharCode bgra_code = 0;
	bgra_code = ('B' << 24) | ('G' << 16) | ('R' << 8) | 'A';

	const bool l10r = pf == l10r_code;
	if (pf == 0)
		blog(LOG_ERROR, "Invalid IOSurface Buffer");
	else if ((pf != bgra_code) && !l10r)
		blog(LOG_ERROR, "Unexpected pixel format: %d (%c%c%c%c)", pf,
		     pf >> 24, pf >> 16, pf >> 8, pf);

	const enum gs_color_format color_format = l10r ? GS_R10G10B10A2
						       : GS_BGRA;

	tex->base.device = device;
	tex->base.type = GS_TEXTURE_2D;
	tex->base.format = color_format;
	tex->base.levels = 1;
	tex->base.gl_format = l10r ? GL_BGRA : convert_gs_format(color_format);
	tex->base.gl_internal_format = convert_gs_internal_format(color_format);
	tex->base.gl_type = l10r ? GL_UNSIGNED_INT_2_10_10_10_REV
				 : GL_UNSIGNED_INT_8_8_8_8_REV;
	tex->base.gl_target = GL_TEXTURE_RECTANGLE_ARB;
	tex->base.is_dynamic = false;
	tex->base.is_render_target = false;
	tex->base.gen_mipmaps = false;
	tex->width = (uint32_t)IOSurfaceGetWidth(ref);
	tex->height = (uint32_t)IOSurfaceGetHeight(ref);

	if (!gl_gen_textures(1, &tex->base.texture))
		goto fail;

	if (!gl_bind_texture(tex->base.gl_target, tex->base.texture))
		goto fail;

	CGLError err = CGLTexImageIOSurface2D(
		[[NSOpenGLContext currentContext] CGLContextObj],
		tex->base.gl_target, tex->base.gl_internal_format, tex->width,
		tex->height, tex->base.gl_format, tex->base.gl_type, ref, 0);

	if (err != kCGLNoError) {
		blog(LOG_ERROR,
		     "CGLTexImageIOSurface2D: %u, %s"
		     " (device_texture_create_from_iosurface)",
		     err, CGLErrorString(err));

		gl_success("CGLTexImageIOSurface2D");
		goto fail;
	}

	if (!gl_tex_param_i(tex->base.gl_target, GL_TEXTURE_MAX_LEVEL, 0))
		goto fail;

	if (!gl_bind_texture(tex->base.gl_target, 0))
		goto fail;

	return (gs_texture_t *)tex;

fail:
	gs_texture_destroy((gs_texture_t *)tex);
	blog(LOG_ERROR, "device_texture_create_from_iosurface (GL) failed");
	return NULL;
}

gs_texture_t *device_texture_open_shared(gs_device_t *device, uint32_t handle)
{
	gs_texture_t *texture = NULL;
	IOSurfaceRef ref = IOSurfaceLookupFromMachPort((mach_port_t)handle);
	texture = device_texture_create_from_iosurface(device, ref);
	CFRelease(ref);
	return texture;
}

bool device_shared_texture_available(void)
{
	return true;
}

bool gs_texture_rebind_iosurface(gs_texture_t *texture, void *iosurf)
{
	if (!texture)
		return false;

	if (!iosurf)
		return false;

	FourCharCode l10r_code = 0;
	l10r_code = ('l' << 24) | ('1' << 16) | ('0' << 8) | 'r';

	FourCharCode bgra_code = 0;
	bgra_code = ('B' << 24) | ('G' << 16) | ('R' << 8) | 'A';

	struct gs_texture_2d *tex = (struct gs_texture_2d *)texture;
	IOSurfaceRef ref = (IOSurfaceRef)iosurf;

	OSType pf = IOSurfaceGetPixelFormat(ref);
	if (pf == 0) {
		blog(LOG_ERROR, "Invalid IOSurface buffer");
	} else if ((pf != bgra_code) && (pf != l10r_code)) {
		blog(LOG_ERROR, "Unexpected pixel format: %d (%c%c%c%c)", pf,
		     pf >> 24, pf >> 16, pf >> 8, pf);
	}

	tex->width = (uint32_t)IOSurfaceGetWidth(ref);
	tex->height = (uint32_t)IOSurfaceGetHeight(ref);

	if (!gl_bind_texture(tex->base.gl_target, tex->base.texture))
		return false;

	CGLError err = CGLTexImageIOSurface2D(
		[[NSOpenGLContext currentContext] CGLContextObj],
		tex->base.gl_target, tex->base.gl_internal_format, tex->width,
		tex->height, tex->base.gl_format, tex->base.gl_type, ref, 0);

	if (err != kCGLNoError) {
		blog(LOG_ERROR,
		     "CGLTexImageIOSurface2D: %u, %s"
		     " (gs_texture_rebind_iosurface)",
		     err, CGLErrorString(err));

		gl_success("CGLTexImageIOSurface2D");
		return false;
	}

	if (!gl_bind_texture(tex->base.gl_target, 0))
		return false;

	return true;
}
