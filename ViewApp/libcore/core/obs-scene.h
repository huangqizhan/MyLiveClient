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

#include "obs.h"
#include "graphics/matrix4.h"

/* how obs scene! */

struct item_action {
	bool visible;
	uint64_t timestamp;
};
/*
 一个scene下会有多个scene_item  每一个scene_item代表一种source(源)
 ex:image_source
    scene_source (此时scene->is_group == true && item->is_group == true)
 */
struct obs_scene_item {
	volatile long ref;
	volatile bool removed;
    ///是否是分组item
	bool is_group;
    ///是否需要更新当前item的变换矩阵的标记
	bool update_transform;
    
	bool update_group_resize;

	int64_t id;
    
    ///当前item如果在group_item下则parent是group_scene  如果是其他的scene_item则parent是一般的scene
	struct obs_scene *parent;
    ///当前item如果在group_item下则parent是group_scene_source  如果是其他的scene_item则parent是一般的scene_source
	struct obs_source *source;
    ///是否active
	volatile long active_refs;
	volatile long defer_update;
	volatile long defer_group_resize;
	
    ///对用户是否是可见的(点击可见不可见按钮)
    bool user_visible;
    ///当是音频source的时候此时不可见 视频可见
	bool visible;
    
    
	bool selected;
	bool locked;

	gs_texrender_t *item_render;
	struct obs_sceneitem_crop crop;
    
    ///当有边框类型时 pos代表在边框的锚点在parent上的位置     当没有边框时pos代表在当前scene_item的锚点在parent上的位置
	struct vec2 pos;
	struct vec2 scale;
	float rot;
    ///此处的对齐方式只是用来定位当前item的旋转中心 (bottom|right 此时item的旋转中心就是item的右下角 也就是说右下角就是锚点)
    /*
     当对图层进行平移、旋转或缩放操作时,变换都是以锚点为参考点进行的。
     改变锚点的位置,可以改变图层变换的行为。例如将锚点设置在图层的左上角,旋转操作就会以左上角为中心进行。
     */
	uint32_t align;

	/* last width/height of the source, this is used to check whether
	 * the transform needs updating */
	uint32_t last_width;
	uint32_t last_height;

	struct vec2 output_scale;
	enum obs_scale_type scale_filter;

	enum obs_blending_method blend_method;
	enum obs_blending_type blend_type;
    
    ///当前item边框的变换矩阵可以变换到父空间  如果需要变换到自己本身空间则求逆
	struct matrix4 box_transform;
	struct vec2 box_scale;
    ///当前item的变换矩阵可以变换到父空间  如果需要变换到自己本身空间则求逆
	struct matrix4 draw_transform;
    /**
     当前source会有一个放置的边框大小 source会放置在source内 并按照边框类型和对齐方式放置source
     */
    ///边框的类型
	enum obs_bounds_type bounds_type;
	uint32_t bounds_align;
    ///边框的大小（宽高）
	struct vec2 bounds;
    ///显示隐藏快捷键
	obs_hotkey_pair_id toggle_visibility;

	obs_data_t *private_settings;

	pthread_mutex_t actions_mutex;
	DARRAY(struct item_action) audio_actions;

    ///当前sourceitem的隐藏显示动画source 及时长 
	struct obs_source *show_transition;
	struct obs_source *hide_transition;
	uint32_t show_transition_duration;
	uint32_t hide_transition_duration;

	/* would do **prev_next, but not really great for reordering 
     每一个scene 会对应一个scene_item列表
     */
	struct obs_scene_item *prev;
	struct obs_scene_item *next;
};
/*
 
 有两种类型的scene 1:一般的scene 2:group scene 
 从source层面来看  scene也是一种source 最终也会存储在 obs_data下的source hash表中
 从UI层面来看 每一个scene(场景)对应多个scene_item(源)
 scene列表回被UI层持有
 */
struct obs_scene {
	struct obs_source *source;
    ///当前的scene_source多对应的scene_item是否是group
    ///一般scene只是在scene列表此时对应的source为scene_source
    ///scene也可能在source列表此时对应的source为scene_group_source 
	bool is_group;
	bool custom_size;
	uint32_t cx;
	uint32_t cy;

	int64_t id_counter;

	pthread_mutex_t video_mutex;
	pthread_mutex_t audio_mutex;
	struct obs_scene_item *first_item;
};
