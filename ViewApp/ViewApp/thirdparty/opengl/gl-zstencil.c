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

#include "gl-subsystem.h"
//创建渲染缓冲对象
/*
 渲染缓冲对象(Renderbuffer Object)是在纹理之后引入到OpenGL中，作为一个可用的帧缓冲附件类型的，所以在过去纹理是唯一可用的附件。和纹理图像一样，渲染缓冲对象是一个真正的缓冲，即一系列的字节、整数、像素等。渲染缓冲对象附加的好处是，它会将数据储存为OpenGL原生的渲染格式，它是为离屏渲染到帧缓冲优化过的。
 渲染缓冲对象直接将所有的渲染数据储存到它的缓冲中，不会做任何针对纹理格式的转换，让它变为一个更快的可写储存介质。然而，渲染缓冲对象通常都是只写的，所以你不能读取它们（比如使用纹理访问）。当然你仍然还是能够使用glReadPixels来读取它，这会从当前绑定的帧缓冲，而不是附件本身，中返回特定区域的像素。

 因为它的数据已经是原生的格式了，当写入或者复制它的数据到其它缓冲中时是非常快的。所以，交换缓冲这样的操作在使用渲染缓冲对象时会非常快。我们在每个渲染迭代最后使用的glfwSwapBuffers，也可以通过渲染缓冲对象实现：只需要写入一个渲染缓冲图像，并在最后交换到另外一个渲染缓冲就可以了。渲染缓冲对象对这种操作非常完美。
 
 */
static bool gl_init_zsbuffer(struct gs_zstencil_buffer *zs, uint32_t width,
			     uint32_t height)
{
	glGenRenderbuffers(1, &zs->buffer);
	if (!gl_success("glGenRenderbuffers"))
		return false;

	if (!gl_bind_renderbuffer(GL_RENDERBUFFER, zs->buffer))
		return false;

	glRenderbufferStorage(GL_RENDERBUFFER, zs->format, width, height);
	if (!gl_success("glRenderbufferStorage"))
		return false;

	gl_bind_renderbuffer(GL_RENDERBUFFER, 0);
	return true;
}

static inline GLenum get_attachment(enum gs_zstencil_format format)
{
	switch (format) {
	case GS_Z16:
		return GL_DEPTH_ATTACHMENT;
	case GS_Z24_S8:
		return GL_DEPTH_STENCIL_ATTACHMENT;
	case GS_Z32F:
		return GL_DEPTH_ATTACHMENT;
	case GS_Z32F_S8X24:
		return GL_DEPTH_STENCIL_ATTACHMENT;
	case GS_ZS_NONE:
		return 0;
	}

	return 0;
}

gs_zstencil_t *device_zstencil_create(gs_device_t *device, uint32_t width,
				      uint32_t height,
				      enum gs_zstencil_format format)
{
	struct gs_zstencil_buffer *zs;

	zs = bzalloc(sizeof(struct gs_zstencil_buffer));
	zs->format = convert_zstencil_format(format);
	zs->attachment = get_attachment(format);
	zs->device = device;

	if (!gl_init_zsbuffer(zs, width, height)) {
		blog(LOG_ERROR, "device_zstencil_create (GL) failed");
		gs_zstencil_destroy(zs);
		return NULL;
	}

	return zs;
}

void gs_zstencil_destroy(gs_zstencil_t *zs)
{
	if (zs) {
		if (zs->buffer) {
			glDeleteRenderbuffers(1, &zs->buffer);
			gl_success("glDeleteRenderbuffers");
		}

		bfree(zs);
	}
}
