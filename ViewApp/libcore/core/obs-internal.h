/******************************************************************************
    Copyright (C) 2013-2014 by Hugh Bailey <obs.jim@gmail.com>

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

#include "util/c99defs.h"
#include "util/darray.h"
#include "util/circlebuf.h"
#include "util/dstr.h"
#include "util/threading.h"
#include "util/platform.h"
#include "util/profiler.h"
#include "util/task.h"
#include "util/uthash.h"
#include "callback/osignal.h"
#include "callback/proc.h"

#include "graphics/graphics.h"
#include "graphics/matrix4.h"

#include "media-io/audio-resampler.h"
#include "media-io/video-io.h"
#include "media-io/audio-io.h"

#include "obs.h"

#include <caption/caption.h>

/* Custom helpers for the UUID hash table */
#define HASH_FIND_UUID(head, uuid, out) \
	HASH_FIND(hh_uuid, head, uuid, UUID_STR_LENGTH, out)
#define HASH_ADD_UUID(head, uuid_field, add) \
	HASH_ADD(hh_uuid, head, uuid_field[0], UUID_STR_LENGTH, add)

#define NUM_TEXTURES 2
#define NUM_CHANNELS 3
#define MICROSECOND_DEN 1000000
#define NUM_ENCODE_TEXTURES 10
#define NUM_ENCODE_TEXTURE_FRAMES_TO_WAIT 1
///===== 把dts转成微秒
static inline int64_t packet_dts_usec(struct encoder_packet *packet)
{
	return packet->dts * MICROSECOND_DEN / packet->timebase_den;
}

struct tick_callback {
	void (*tick)(void *param, float seconds);
	void *param;
};

struct draw_callback {
	void (*draw)(void *param, uint32_t cx, uint32_t cy);
	void *param;
};

struct rendered_callback {
	void (*rendered)(void *param);
	void *param;
};

/* ------------------------------------------------------------------------- */
/* validity checks */

static inline bool obs_object_valid(const void *obj, const char *f,
				    const char *t)
{
	if (!obj) {
		blog(LOG_DEBUG, "%s: Null '%s' parameter", f, t);
		return false;
	}

	return true;
}

#define obs_ptr_valid(ptr, func) obs_object_valid(ptr, func, #ptr)
#define obs_source_valid obs_ptr_valid
#define obs_output_valid obs_ptr_valid
#define obs_encoder_valid obs_ptr_valid
#define obs_service_valid obs_ptr_valid


#pragma mark 插件
struct obs_module {
	char *mod_name;
	const char *file;
	char *bin_path;
	char *data_path;
    ///lib库指针
	void *module;
	bool loaded;

	bool (*load)(void);
	void (*unload)(void);
	void (*post_load)(void);
	void (*set_locale)(const char *locale);
	bool (*get_string)(const char *lookup_string,
			   const char **translated_string);
	void (*free_locale)(void);
	uint32_t (*ver)(void);
	void (*set_pointer)(obs_module_t *module);
	const char *(*name)(void);
	const char *(*description)(void);
	const char *(*author)(void);

	struct obs_module *next;
};

extern void free_module(struct obs_module *mod);

struct obs_module_path {
	char *bin;
	char *data;
};

static inline void free_module_path(struct obs_module_path *omp)
{
	if (omp) {
		bfree(omp->bin);
		bfree(omp->data);
	}
}

static inline bool check_path(const char *data, const char *path,
			      struct dstr *output)
{
	dstr_copy(output, path);
	dstr_cat(output, data);

	return os_file_exists(output->array);
}

/* ------------------------------------------------------------------------- */
/* hotkeys */
///单个快捷键
struct obs_hotkey {
	obs_hotkey_id id;
	char *name;
	char *description;

	obs_hotkey_func func;
	void *data;
	int pressed;

	obs_hotkey_registerer_t registerer_type;
	void *registerer;

	obs_hotkey_id pair_partner_id;

	UT_hash_handle hh;
};
///成对快捷键
struct obs_hotkey_pair {
	obs_hotkey_pair_id pair_id;
	obs_hotkey_id id[2];
	obs_hotkey_active_func func[2];
	bool pressed0;
	bool pressed1;
	void *data[2];

	UT_hash_handle hh;
};

typedef struct obs_hotkey_pair obs_hotkey_pair_t;

typedef struct obs_hotkeys_platform obs_hotkeys_platform_t;

void *obs_hotkey_thread(void *param);

struct obs_core_hotkeys;
bool obs_hotkeys_platform_init(struct obs_core_hotkeys *hotkeys);
void obs_hotkeys_platform_free(struct obs_core_hotkeys *hotkeys);
bool obs_hotkeys_platform_is_pressed(obs_hotkeys_platform_t *context,
                                     obs_key_t key);

const char *obs_get_hotkey_translation(obs_key_t key, const char *def);

struct obs_context_data;
void obs_hotkeys_context_release(struct obs_context_data *context);

void obs_hotkeys_free(void);

struct obs_hotkey_binding {
    //key modifier
	obs_key_combination_t key;
	bool pressed;
	bool modifiers_match;

	obs_hotkey_id hotkey_id;
    //回调
	obs_hotkey_t *hotkey;
};

struct obs_hotkey_name_map_item;
void obs_hotkey_name_map_free(void);

/* ------------------------------------------------------------------------- */
/**
 obs主画布的source都是以transition_source开始
 */
//主画布和预览画布都对应一个obs_view
struct obs_view {
	pthread_mutex_t channels_mutex;
    ///视频： 转场source->scene_source->input_sources   当前选中的转场source
    ///音频： 桌面音频source 、
	obs_source_t *channels[MAX_CHANNELS];
};

extern bool obs_view_init(struct obs_view *view);
extern void obs_view_free(struct obs_view *view);

/* ------------------------------------------------------------------------- */
/* displays */
/*
 一个widget 代表一个display  一个display持有一个swap
 渲染线程会遍历所有的display 并且在渲染每一个display时会把display的swap切换到device->cur_swap
 
 */
struct obs_display {
	bool update_color_space;
	bool enabled;
	uint32_t cx, cy;
	uint32_t next_cx, next_cy;
	uint32_t background_color;
    ///主窗口的渲染/各种预览的渲染 之间切换
	gs_swapchain_t *swap;
	pthread_mutex_t draw_callbacks_mutex;
	pthread_mutex_t draw_info_mutex;
	DARRAY(struct draw_callback) draw_callbacks;
	bool use_clear_workaround;

	struct obs_display *next;
	struct obs_display **prev_next;
};

extern bool obs_display_init(struct obs_display *display,
			     const struct gs_init_data *graphics_data);
extern void obs_display_free(struct obs_display *display);

/* ------------------------------------------------------------------------- */
/* core */

struct obs_vframe_info {
	uint64_t timestamp;
	int count;
};

struct obs_tex_frame {
	gs_texture_t *tex;
	gs_texture_t *tex_uv;
	uint32_t handle;
	uint64_t timestamp;
	uint64_t lock_key;
	int count;
	bool released;
};

struct obs_task_info {
	obs_task_t task;
	void *param;
};
/*
 ///每一个画布对应一个obs_core_video_mix和一个线程
 画布上会有多个source 最终将多个source混合 并输出  (多个source在view->channels里)
 */
struct obs_core_video_mix {
	struct obs_view *view;
    ///cur_texture 对应当前纹理
    ///
	gs_stagesurf_t *active_copy_surfaces[NUM_TEXTURES][NUM_CHANNELS];
	gs_stagesurf_t *copy_surfaces[NUM_TEXTURES][NUM_CHANNELS];
	gs_texture_t *convert_textures[NUM_CHANNELS];
#ifdef _WIN32
	gs_stagesurf_t *copy_surfaces_encode[NUM_TEXTURES];
	gs_texture_t *convert_textures_encode[NUM_CHANNELS];
#endif
    ///绘制的数据会存储在这个纹理上
	gs_texture_t *render_texture;
    ///有输出时 输出到output_texture
	gs_texture_t *output_texture;
	enum gs_color_space render_space;
	bool texture_rendered;
	bool textures_copied[NUM_TEXTURES];
	bool texture_converted;
    
    ///nv12纹理
	bool using_nv12_tex;
    ///p010纹理
	bool using_p010_tex;
    ///从GPU中读出frame 存储到此处
	struct circlebuf vframe_info_buffer;
	struct circlebuf vframe_info_buffer_gpu;
	gs_stagesurf_t *mapped_surfaces[NUM_CHANNELS];
	int cur_texture;

    /*
     两对状态
     raw_active、 raw_was_active
     gpu_was_active、 gpu_encoder_active
     was_active、active
     每次tick 都会更新状态
     当原始状态跟当前状态不一样是 清空数据
     */
	volatile long raw_active;         ///会在其他线程修改 故用volatile修饰
    volatile long gpu_encoder_active; ///会在其他线程修改 故用volatile修饰
    bool raw_was_active;
    bool gpu_was_active;
	bool was_active;
    
    
    
    
	pthread_mutex_t gpu_encoder_mutex;
	struct circlebuf gpu_encoder_queue;
	struct circlebuf gpu_encoder_avail_queue;
	DARRAY(obs_encoder_t *) gpu_encoders;
    ///GPU编码器输入帧信号
	os_sem_t *gpu_encode_semaphore;
    ///等待gpu编码结束
	os_event_t *gpu_encode_inactive;
	pthread_t gpu_encode_thread;
	bool gpu_encode_thread_initialized;
	volatile bool gpu_encode_stop;
    //每一路视频的输出相关的操作
	video_t *video;
    //每一路视频的输出info
	struct obs_video_info ovi;
    /** Use shaders to convert to different color formats */
	bool gpu_conversion;
	const char *conversion_techs[NUM_CHANNELS];
	bool conversion_needed;
	float conversion_width_i;
	float conversion_height_i;
    ///空间色彩转换矩阵
	float color_matrix[16];
};

extern struct obs_core_video_mix *obs_create_video_mix(struct obs_video_info *ovi);
extern void obs_free_video_mix(struct obs_core_video_mix *video);
///所有的视频相关
struct obs_core_video {
	graphics_t *graphics;
	gs_effect_t *default_effect;
	gs_effect_t *default_rect_effect;
	gs_effect_t *opaque_effect;
	gs_effect_t *solid_effect;
	gs_effect_t *repeat_effect;
	gs_effect_t *conversion_effect;
	gs_effect_t *bicubic_effect;
	gs_effect_t *lanczos_effect;
	gs_effect_t *area_effect;
	gs_effect_t *bilinear_lowres_effect;
	gs_effect_t *premultiplied_alpha_effect;
	gs_samplerstate_t *point_sampler;
    // video线程下一次的tick时间
	uint64_t video_time;
    //帧率
	uint64_t video_frame_interval_ns;
    //帧率一半
	uint64_t video_half_frame_interval_ns;
    ///1秒内每次tick的时长
	uint64_t video_avg_frame_time_ns;
	double video_fps;
	pthread_t video_thread;
	uint32_t total_frames;
    ///延迟的帧数
	uint32_t lagged_frames;
	bool thread_initialized;

	gs_texture_t *transparent_texture;

    ///去隔行扫描effect 
	gs_effect_t *deinterlace_discard_effect;
	gs_effect_t *deinterlace_discard_2x_effect;
	gs_effect_t *deinterlace_linear_effect;
	gs_effect_t *deinterlace_linear_2x_effect;
	gs_effect_t *deinterlace_blend_effect;
	gs_effect_t *deinterlace_blend_2x_effect;
	gs_effect_t *deinterlace_yadif_effect;
	gs_effect_t *deinterlace_yadif_2x_effect;

    /*
     SDR:
     SDR是标准的动态范围,通常指8位色深,亮度范围为0-100 nits。
     SDR视频的亮度和色彩表现有一定局限性,无法表现高对比度和广色域。
     */
	float sdr_white_level;
    /*
     HDR:
     HDR是高动态范围,通常指10位或12位色深,亮度范围可达1000 nits或更高。
     HDR视频可以表现出更广阔的色域和更高的对比度,从而带来更出色的视觉体验。
     */
	float hdr_nominal_peak_level;

	pthread_mutex_t task_mutex;
	struct circlebuf tasks;

	pthread_mutex_t mixes_mutex;
    ///所有画布(目前看到mixes中只有main_mix)
	DARRAY(struct obs_core_video_mix *) mixes;
    ///主画布
	struct obs_core_video_mix *main_mix;
};

struct audio_monitor;

struct obs_core_audio {
	audio_t *audio;

	DARRAY(struct obs_source *) render_order;
	DARRAY(struct obs_source *) root_nodes;
    ///开始缓冲时的 时钟时间 
	uint64_t buffered_ts;
	struct circlebuf buffered_timestamps;
    ///当前音频输出线程正在缓冲  需要等待的tick次数 
	uint64_t buffering_wait_ticks;
    ///当前缓冲区的时长 （单位 clock tick）  每次tick写入1024个采样 
	int total_buffering_ticks;
    ///缓冲区最大时长 （单位 clock tick）
	int max_buffering_ticks;
    ///固定缓冲区大小
	bool fixed_buffer;
    
	pthread_mutex_t monitoring_mutex;
	DARRAY(struct audio_monitor *) monitors;
	char *monitoring_device_name;
	char *monitoring_device_id;

	pthread_mutex_t task_mutex;
	struct circlebuf tasks;
};

/* user sources, output channels, and displays */
struct obs_core_data {
	/* Hash tables (uthash) */
  
    ///hash表的表头
    
    ///存储所有的source
	struct obs_source *sources;        /* Lookup by UUID (hh_uuid) */
    ///存储所有公开的source
	struct obs_source *public_sources; /* Lookup by name (hh) */

    
    
	/*source Linked lists */
	struct obs_source *first_audio_source;
    ///每一个画布对应一个display回调
	struct obs_display *first_display;
    /*output Linked lists */
	struct obs_output *first_output;
	struct obs_encoder *first_encoder;
	struct obs_service *first_service;

	pthread_mutex_t sources_mutex;
	pthread_mutex_t displays_mutex;
	pthread_mutex_t outputs_mutex;
	pthread_mutex_t encoders_mutex;
	pthread_mutex_t services_mutex;
	pthread_mutex_t audio_sources_mutex;
	pthread_mutex_t draw_callbacks_mutex;
    ///会之前回调
	DARRAY(struct draw_callback) draw_callbacks;
    ///会之后回调
	DARRAY(struct rendered_callback) rendered_callbacks;
	DARRAY(struct tick_callback) tick_callbacks;
    ///主画布
	struct obs_view main_view;

	long long unnamed_index;

	obs_data_t *private_data;

	volatile bool valid;

	DARRAY(char *) protocols;
	DARRAY(obs_source_t *) sources_to_tick;
};

/* user hotkeys
 全局的快捷键
 */
struct obs_core_hotkeys {
	pthread_mutex_t mutex;
    ///单个快捷键
	obs_hotkey_t *hotkeys;
    ///每注册一个单个快捷键就+1
	obs_hotkey_id next_id;
    ///成对快捷键
	obs_hotkey_pair_t *hotkey_pairs;
    ///每注册一个对快捷键就+1
	obs_hotkey_pair_id next_pair_id;

	pthread_t hotkey_thread;
	bool hotkey_thread_initialized;
	os_event_t *stop_event;
	bool thread_disable_press;
	bool strict_modifiers;
	bool reroute_hotkeys;
	DARRAY(obs_hotkey_binding_t) bindings;

	obs_hotkey_callback_router_func router_func;
	void *router_func_data;

	obs_hotkeys_platform_t *platform_context;

	pthread_once_t name_map_init_token;
	struct obs_hotkey_name_map_item *name_map;

	signal_handler_t *signals;

	char *translations[OBS_KEY_LAST_VALUE];
	char *mute;             ///静音
	char *unmute;           ///取消静音
	char *push_to_mute;     ///按住静音
	char *push_to_talk;     ///按住讲话
    
    ///每一个scene现实与隐藏的模版
	char *sceneitem_show;   ///显示场景
	char *sceneitem_hide;   ///隐藏场景
};

struct obs_core {
	struct obs_module *first_module;
    //插件的所有来源路径 (1 /Library/Application Support/obs-studio 2 /Users/xx//Library/Application Support/obs-studio  3 自带的 在安装包中NSBundle  /Contens/plugins/)
	DARRAY(struct obs_module_path) module_paths;
    ///所有注册的source类型
	DARRAY(struct obs_source_info) source_types;
    //输入类型
	DARRAY(struct obs_source_info) input_types;
    //滤镜类型
	DARRAY(struct obs_source_info) filter_types;
    //转场类型
	DARRAY(struct obs_source_info) transition_types;
    //输出类型
	DARRAY(struct obs_output_info) output_types;
    ///编码器
	DARRAY(struct obs_encoder_info) encoder_types;
	DARRAY(struct obs_service_info) service_types;
//	DARRAY(struct obs_modal_ui) modal_ui_callbacks;
//	DARRAY(struct obs_modeless_ui) modeless_ui_callbacks;
    //信号处理器
	signal_handler_t *signals;
    ///匿名的回调事件器
	proc_handler_t *procs;
    //本地语言
	char *locale;
    //插件配置路径
	char *module_config_path;
    //内部变量profiler_name_store_t 是否是内部创建的
	bool name_store_owned;
	profiler_name_store_t *name_store;

	/* segmented into multiple sub-structures to keep things a bit more
	 * clean and organized
     音频视频分别在audio_io_thread video_io_thread 渲染并采集 
     */
	struct obs_core_video video;
	struct obs_core_audio audio;
    
	struct obs_core_data data;
	struct obs_core_hotkeys hotkeys;
    ///此队列有单独线程执行 (销毁source使用)
	os_task_queue_t *destruction_task_thread;
	obs_task_handler_t ui_task_handler;
};

extern struct obs_core *obs;

struct obs_graphics_context {
    // video线程上一次的tick时间
    uint64_t video_time;
	uint64_t last_time;
    //线程tick时间  1/fps
	uint64_t interval;
    //该线程每秒tick的帧数 所用的总时间
	uint64_t frame_time_total_ns;
	uint64_t fps_total_ns;
    //该线程每秒tick的帧数
	uint32_t fps_total_frames;
	const char *video_thread_name;
};

///====所有source 画布绘制 以及画布的输出 (输出到每个画布对应的video_output中)
extern void *obs_graphics_thread(void *param);
extern bool obs_graphics_thread_loop(struct obs_graphics_context *context);
#ifdef __APPLE__
extern void *obs_graphics_thread_autorelease(void *param);
extern bool
obs_graphics_thread_loop_autorelease(struct obs_graphics_context *context);
#endif

extern gs_effect_t *obs_load_effect(gs_effect_t **effect, const char *file);

///音频IO线程获取音频数据回调 
extern bool audio_callback(void *param, uint64_t start_ts_in,
			   uint64_t end_ts_in, uint64_t *out_ts,
			   uint32_t mixers, struct audio_output_data *mixes);

extern struct obs_core_video_mix *get_mix_for_video(video_t *video);

extern void start_raw_video(video_t *video, const struct video_scale_info *conversion,
		void (*callback)(void *param, struct video_data *frame),
		void *param);
extern void stop_raw_video(video_t *video,
			   void (*callback)(void *param,
					    struct video_data *frame),
			   void *param);

/* ------------------------------------------------------------------------- */
/* obs shared context data
 volatile 在 C/C++ 中是一个 类型修饰符，它告诉编译器：这个变量可能随时被程序之外的因素修改，所以编译器不能对它做某些优化。
 对 volatile 变量，每次访问都必须从内存中读取，每次写入都必须写回内存
 */
///即使有weak_refs > 0,只要refs = 0对象也可以被销毁
struct obs_weak_ref {
	volatile long refs;      ///强引用计数 参与对象的声明周期
	volatile long weak_refs; ///弱引用计数 不参与对象的声明周期
};
///弱引用对象
struct obs_weak_object {
	struct obs_weak_ref ref;
	struct obs_context_data *object;
};

typedef void (*obs_destroy_cb)(void *obj);
//此机构体会已对象的形式切套在其他结构体中
struct obs_context_data {
	char *name;
	const char *uuid;
    /*
     ex: source 
     (source_transition_fade 此时data是transtion_fade内部的fade_info)
     (source_scene 此时data是obs_scene内部的obs_scene)
     */
	void *data;
    /**
     在初始化source时可以从外部 创建source并传入 并带上部分配置
     当调用source_info.get_defaults 会获取每种source的默认设置 
     settings 是对data对象的一些默认设置
     */
	obs_data_t *settings;
    
	signal_handler_t *signals;
	proc_handler_t *procs;
	enum obs_obj_type type;

	struct obs_weak_object *control;
	obs_destroy_cb destroy;

	DARRAY(obs_hotkey_id) hotkeys;
	DARRAY(obs_hotkey_pair_id) hotkey_pairs;
	obs_data_t *hotkey_data;
    ///改过名字的缓存
	DARRAY(char *) rename_cache;
	pthread_mutex_t rename_cache_mutex;

	pthread_mutex_t *mutex;
    
    ///类似双向链表 (方便插入和删除)
	struct obs_context_data *next;        ///下一个节点
	struct obs_context_data **prev_next;  ///上一个节点的next地址 也就是二级指针
/*
 void delete_node(struct obs_context_data *node) {
     if (node->prev_next) {
         *node->prev_next = node->next;              // 更新前一个节点的 `next` 指针
          
         if (node->next) {
             node->next->prev_next = node->prev_next; // 更新下一个节点的 `prev_next` 指针
         }
         free(node); // 释放当前节点
     }
 }
 
 */
    
    //此处的两个hash 不是在context_data中使用的 而是在obs_source obs_encoder ...中使用
	UT_hash_handle hh;
	UT_hash_handle hh_uuid;

	bool private;
};

extern bool obs_context_data_init(struct obs_context_data *context,
				  enum obs_obj_type type, obs_data_t *settings,
				  const char *name, const char *uuid,
				  obs_data_t *hotkey_data, bool private);
extern void obs_context_init_control(struct obs_context_data *context,
				     void *object, obs_destroy_cb destroy);
extern void obs_context_data_free(struct obs_context_data *context);

extern void obs_context_data_insert(struct obs_context_data *context,
				    pthread_mutex_t *mutex, void *first);
extern void obs_context_data_insert_name(struct obs_context_data *context,
					 pthread_mutex_t *mutex, void *first);
///
extern void obs_context_data_insert_uuid(struct obs_context_data *context,
					 pthread_mutex_t *mutex,
					 void *first_uuid);

extern void obs_context_data_remove(struct obs_context_data *context);
extern void obs_context_data_remove_name(struct obs_context_data *context,
					 void *phead);
extern void obs_context_data_remove_uuid(struct obs_context_data *context,
					 void *puuid_head);

extern void obs_context_wait(struct obs_context_data *context);

extern void obs_context_data_setname(struct obs_context_data *context,
				     const char *name);
extern void obs_context_data_setname_ht(struct obs_context_data *context,
					const char *name, void *phead);

/* ------------------------------------------------------------------------- */
/* ref-counting  */
///====直接增加引用计数
static inline void obs_ref_addref(struct obs_weak_ref *ref)
{
	os_atomic_inc_long(&ref->refs);
}
///====直接减小引用计数
static inline bool obs_ref_release(struct obs_weak_ref *ref)
{
	return os_atomic_dec_long(&ref->refs) == -1;
}
///====只要ref没有销毁则引用计数一定+1
static inline bool obs_weak_ref_get_ref(struct obs_weak_ref *ref)
{
    ///获取旧值（此处是原子操作 同一时间只能有一个线程执行）
	long owners = os_atomic_load_long(&ref->refs);
    ///只要保证此时ref没有销毁(owners > -1)  使用while循环保证ref->refs 一定+1
	while (owners > -1) {
		if (os_atomic_compare_exchange_long(&ref->refs, &owners,
						    owners + 1)) {
			return true;
		}
	}

	return false;
}
///====直接增加弱引用计数
static inline void obs_weak_ref_addref(struct obs_weak_ref *ref)
{
    os_atomic_inc_long(&ref->weak_refs);
}
///====直接减小弱引用计数
static inline bool obs_weak_ref_release(struct obs_weak_ref *ref)
{
    return os_atomic_dec_long(&ref->weak_refs) == -1;
}

static inline bool obs_weak_ref_expired(struct obs_weak_ref *ref)
{
	long owners = os_atomic_load_long(&ref->refs);
	return owners < 0;
}

/* ------------------------------------------------------------------------- */
/* sources  */

struct async_frame {
	struct obs_source_frame *frame;
	long unused_count;
	bool used;
};

enum audio_action_type {
	AUDIO_ACTION_VOL,
	AUDIO_ACTION_MUTE,
    //push_to_talk Push-to-Talk(按住说话)动作
	AUDIO_ACTION_PTT,
    //push_to_mute Push-to-Mute(按住静音)动作
	AUDIO_ACTION_PTM,
};

struct audio_action {
	uint64_t timestamp;
	enum audio_action_type type;
	union {
		float vol;
		bool set;
	};
};

struct obs_weak_source {
	struct obs_weak_ref ref;
	struct obs_source *source;
};

struct audio_cb_info {
	obs_source_audio_capture_t callback;
	void *param;
};

struct caption_cb_info {
	obs_source_caption_t callback;
	void *param;
};

/**
 所有的source在内存中都在一个hash表里
 一个source可能同时包含音频和视频
 **/
struct obs_source {
	struct obs_context_data context;
    ///每一种source已经注册的类型信息
	struct obs_source_info info;

	/* general exposed flags that can be set for the source */
	uint32_t flags;
	uint32_t default_flags;
	uint32_t last_obs_ver;

	/* indicates ownership of the info.id buffer */
	bool owns_info_id;

    /* signals to call the source update in the video thread
     每一次视频源的改变（包括source->context.settings中的属性改变）defer_update_count 就会+1
     视频线程的tick回调 重置defer_update_count为0
     如果跟新defer_update_count失败则 表明在更新defer_update_count时视频线程又更新了defer_update_count
     此时在会在下一次的tick操作中重新尝试更新defer_update_count为0
     如果defer_update_count有更新则source->info.update会更新
     */
	long defer_update_count;

	
    ///=====??
    /* ensures show/hide are only called once */
	volatile long show_refs;
	/* ensures activate/deactivate are only called once
     MAIN_VIEW 只有主窗口会使用 （ex: video play/stop  filter working/unworking）
     */
	volatile long activate_refs;
    
    
    
	/* source is in the process of being destroyed */
	volatile long destroying;
    /* used to indicate that the source has been removed and all
	 * references to it should be released (not exactly how I would prefer
	 * to handle things but it's the best option) */
	bool removed;

	/*  used to indicate if the source should show up when queried for user ui */
	bool temp_removed;
    //source的激活状态（ex: video play/stop  filter working/unworking ）
	bool active;
	bool showing;

	/* used to temporarily disable sources if needed 
     临时disable sourse 
     */
	bool enabled;

	/* hint to allow sources to render more quickly 目前没有发现有用的地方
     （当变换矩阵只有平移时 才置为1） */
	bool texcoords_centered;

	/* timing (if video is present, is based upon video)
     计时(如果有视频，则基于视频)
     */
    ///当前的音频source是否设置过了时间同步调整
	volatile bool timing_set;
    ///当前的音频source调整了多长时间 (调整是时钟时间相对于frame.ts的差值进行调整)
	volatile uint64_t timing_adjust;
	
    uint64_t resample_offset;
    ///上一个音频帧的时间
	uint64_t last_audio_ts;
    ///下一个音频帧的最小时间 当前帧的时间+当前帧数的时长  (如果下一帧的时间戳是next_audio_ts_min的话表明此时就是理想状态)
	uint64_t next_audio_ts_min;
    ///下一帧音频帧的时钟时间(next_audio_ts_min + timing_adjust) 理想时间+调整时间
	uint64_t next_audio_sys_ts_min;
    //上一个从帧缓冲区获取到的frame的timestamp
	uint64_t last_frame_ts;
    ///当前tick执行时 上一次tick的时钟时间
	uint64_t last_sys_timestamp;
    ///异步渲染是否已经结束
	bool async_rendered;

	/* audio */
	bool audio_failed;
	bool audio_pending;
    ///标记为true时 表示需等待下一次观察 音频数据是否已经挂起
	bool pending_stop;
	bool audio_active;
	bool user_muted;
	bool muted;
    ///音频source的双向链表
	struct obs_source *next_audio_source;
	struct obs_source **prev_next_audio_source;
    ///音频线程 当前source 理论上当前tick的结束时间 (tick 开始时间 + 1024个采样时长 )  作用是为了同步各个source
	uint64_t audio_ts;
    ///输入到source 处理后的音频数据(包括同步 滤镜) 每个通道一个buffer
	struct circlebuf audio_input_buf[MAX_AUDIO_CHANNELS];
    ///每次tick的时候 都会检查输入缓冲区大小
	size_t last_audio_input_buf_size;
	DARRAY(struct audio_action) audio_actions;
    ///====在输出的场景下 最多有6个轨道 每个轨道最多有8个通道 而每个source可能在不同的轨道上
    ///如果当前source在每个轨道上都存在  则audio_output_buf会保存当前source的音频数据的六个副本
    ///渲染后的数据
	float *audio_output_buf[MAX_AUDIO_MIXES][MAX_AUDIO_CHANNELS];
	float *audio_mix_buf[MAX_AUDIO_CHANNELS];
	struct resample_info sample_info;
	audio_resampler_t *resampler;
	pthread_mutex_t audio_actions_mutex;
	pthread_mutex_t audio_buf_mutex;
	pthread_mutex_t audio_mutex;
	pthread_mutex_t audio_cb_mutex;
	DARRAY(struct audio_cb_info) audio_cb_list;
    ///音频source采集后的数据
	struct obs_audio_data audio_data;
	size_t audio_storage_size;
    ///====当前source所对应的轨道索引（如果选中1 那么在1轨道就会存在该source 最多6个轨道）
	uint32_t audio_mixers;
	float user_volume;
	float volume;
    //同步偏移量
	int64_t sync_offset;
	int64_t last_sync_offset;
    //用于控制左右声道音量比例的重要参数
	float balance;


    /* async video data 
     异步渲染相关 只是渲染之后会存储在缓冲区
     */
    ///当前帧的视频纹理数据 (当渲染隔行扫描时此时为上半场)
	gs_texture_t *async_textures[MAX_AV_PLANES];
    ///当前帧的渲染器 (当渲染隔行扫描时此时为上半场的渲染器)
	gs_texrender_t *async_texrender;
    ///source每次tick时 (缓冲区中的最近一个frame ) （当需要去隔行扫描时表示上半场）
	struct obs_source_frame *cur_async_frame;
    ///每一帧是否需要在GPU层面转换
	bool async_gpu_conversion;
    ///当前帧的format 
	enum video_format async_format;
    ///当前帧是否fullrange 
	bool async_full_range;
	uint8_t async_trc;
	enum video_format async_cache_format;
	bool async_cache_full_range;
	uint8_t async_cache_trc;
	enum gs_color_format async_texture_formats[MAX_AV_PLANES];
	int async_channel_count;
	long async_rotation;
	bool async_flip;
	bool async_linear_alpha;
    ///当前缓存区是否有数据
	bool async_active;
    ///当前source的纹理及纹理渲染器是否已经准备好
	bool async_update_texture;
    //不使用异步缓冲 
	bool async_unbuffered;
	bool async_decoupled;
    //应该是placeholder 
	struct obs_source_frame *async_preload_frame;
    //帧缓冲区（存储可以重复利用）
    DARRAY(struct async_frame) async_cache;
    //帧缓冲区（是对async_cache的引用）
    DARRAY(struct obs_source_frame *) async_frames;
	pthread_mutex_t async_mutex;
    //当前帧的宽高
	uint32_t async_width;
	uint32_t async_height;
    
    ///缓冲区中的frame宽高
	uint32_t async_cache_width;
	uint32_t async_cache_height;
    ///GPU转换宽高 （YUV多个分量）
	uint32_t async_convert_width[MAX_AV_PLANES];
	uint32_t async_convert_height[MAX_AV_PLANES];

    //字幕
	pthread_mutex_t caption_cb_mutex;
	DARRAY(struct caption_cb_info) caption_cb_list;

	/* async video deinterlacing
     当要渲染去隔行扫描 每次会渲染两帧
     */
    //从原始视频帧到去隔行处理后帧的时间差
	uint64_t deinterlace_offset;
    //这个时间戳指的是去隔行处理完成后，该帧应当显示的具体时间点
	uint64_t deinterlace_frame_ts;
    ///去隔行扫描的shader 
	gs_effect_t *deinterlace_effect;
    ///下半场的视频帧
	struct obs_source_frame *prev_async_frame;
    ///下半场的视频数据 (可能会有多个通道YUV)
	gs_texture_t *async_prev_textures[MAX_AV_PLANES];
    ///下半场渲染器
	gs_texrender_t *async_prev_texrender;
	uint32_t deinterlace_half_duration;
	enum obs_deinterlace_mode deinterlace_mode;
    //隔行扫描上下场的顺序
	bool deinterlace_top_first;
	bool deinterlace_rendered;

	/* filters */
    /*
     source(parent)
        /|\      |(target)
         |      \|/
         |       filter1
   target|       |(target)
         |      \|/
         |_______filter2
     */
    //当前source是滤镜类型时此字段有用 表示当前滤镜的持有者 (image_source 下有多个filter_source 每个filter_source->filter_parent == image_source)
	struct obs_source *filter_parent;
    //当前source是滤镜类型时此字段有用 表示当前滤镜处理后输出的下一个滤镜 如果是最后一个滤镜filter_target == image_source
	struct obs_source *filter_target;
    
    //有滤镜需要处理的source才会有次字段
	DARRAY(struct obs_source *) filters;
	pthread_mutex_t filter_mutex;
    
    
    
    ///滤镜渲染器
	gs_texrender_t *filter_texrender;
	enum obs_allow_direct_render allow_direct;
    ///是否正在滤镜处理
	bool rendering_filter;
	bool filter_bypass_active;

	/* sources specific hotkeys */
	obs_hotkey_pair_id mute_unmute_key;
	obs_hotkey_id push_to_mute_key;
	obs_hotkey_id push_to_talk_key;
    //推流声音相关
	bool push_to_mute_enabled;
	bool push_to_mute_pressed;
	bool user_push_to_mute_pressed;
	bool push_to_talk_enabled;
	bool push_to_talk_pressed;
	bool user_push_to_talk_pressed;
	uint64_t push_to_mute_delay;
	uint64_t push_to_mute_stop_time;
	uint64_t push_to_talk_delay;
	uint64_t push_to_talk_stop_time;

	/* transitions 只有当source为 tranition类型的source时 才会有转场相关的数据 */
    ///点击切换不同的scene时transition_start_time会更新为当前的时钟时间
	uint64_t transition_start_time;
    ///本次转场的时长
	uint64_t transition_duration;
    
    ///转场上 转场下的两个纹理渲染器 
	pthread_mutex_t transition_tex_mutex;
	gs_texrender_t *transition_texrender[2];
	
    pthread_mutex_t transition_mutex;
    /*
     在转场前transition_source[0] == scene_A,  active[0] == true 
     Scene A -> Scene B 的转场过程：

     初始状态：
     transition_sources[0] = Scene A
     transition_sources[1] = NULL

     开始转场：
     transition_sources[0] = Scene A  // 保持不变
     transition_sources[1] = Scene B  // 设置目标场景

     转场结束：
     transition_sources[0] = Scene B  // 目标场景成为当前场景
     transition_sources[1] = NULL     // 清空目标位置
     
     
     // 在转场开始时
     source_active[0] = true;   // A场景（起始）
     source_active[1] = false;  // B场景（目标）

     // 转场过程中
     source_active[0] = true;   // A场景仍然活动
     source_active[1] = true;   // B场景也变为活动

     // 转场结束后
     source_active[0] = true;  //B场景也变为活动
     source_active[1] = false; // 只保留B场景活动
     
     */
	obs_source_t *transition_sources[2];
    ///转场过程中的夹紧值。 用于限制转场效果的强度或范围
    float transition_manual_clamp;
    ///转场过程中的扭力值。控制转场的速度或动量。
	float transition_manual_torque;
    ///转场时目标值(淡入淡出0-1)
	float transition_manual_target;
    ///当前时间的转场值(是在时刻变化的 在tick中修改)
	float transition_manual_val;
    
    ///正在进行video audio 专场动画
	bool transitioning_video;
	bool transitioning_audio;
    
	bool transition_source_active[2];
	
    uint32_t transition_alignment;
    ///两个转场source中最大的宽高
	uint32_t transition_actual_cx;
	uint32_t transition_actual_cy;
    ///宽高
	uint32_t transition_cx;
	uint32_t transition_cy;
    
    ///手动修改转场动画的时长
	uint32_t transition_fixed_duration;
	bool transition_use_fixed_duration;
    
    enum obs_transition_mode transition_mode;
    ///转场时的放缩类型
	enum obs_transition_scale_type transition_scale_type;
    ///转场时的放缩平移矩阵
	struct matrix4 transition_matrices[2];

	/* color space */
	gs_texrender_t *color_space_texrender;
    
    //audio ply
	struct audio_monitor *monitor;
	enum obs_monitoring_type monitoring_type;

	obs_data_t *private_settings;
};

extern struct obs_source_info *get_source_info(const char *id);
extern struct obs_source_info *get_source_info2(const char *unversioned_id,
						uint32_t ver);
///===== 初始化obs_context_data 及source对应的singls
extern bool obs_source_init_context(struct obs_source *source,
				    obs_data_t *settings, const char *name,
				    const char *uuid, obs_data_t *hotkey_data,
				    bool private);

extern bool obs_transition_init(obs_source_t *transition);
extern void obs_transition_free(obs_source_t *transition);
///transition source tick  t:连续两次tick的时间差
extern void obs_transition_tick(obs_source_t *transition, float t);
extern void obs_transition_enum_sources(obs_source_t *transition,
					obs_source_enum_proc_t enum_callback,
					void *param);

///存储transition到data
extern void obs_transition_save(obs_source_t *source, obs_data_t *data);
extern void obs_transition_load(obs_source_t *source, obs_data_t *data);



struct audio_monitor *audio_monitor_create(obs_source_t *source);
void audio_monitor_reset(struct audio_monitor *monitor);
extern void audio_monitor_destroy(struct audio_monitor *monitor);

extern obs_source_t *
obs_source_create_set_last_ver(const char *id, const char *name,
			       const char *uuid, obs_data_t *settings,
			       obs_data_t *hotkey_data, uint32_t last_obs_ver,
			       bool is_private);
extern void obs_source_destroy(struct obs_source *source);

enum view_type {
	MAIN_VIEW,  ///主画布
	AUX_VIEW,   ///预览画布
};

static inline void obs_source_dosignal(struct obs_source *source,
				       const char *signal_obs,
				       const char *signal_source)
{
	struct calldata data;
	uint8_t stack[128];

	calldata_init_fixed(&data, stack, sizeof(stack));
	calldata_set_ptr(&data, "source", source);
	if (signal_obs && !source->context.private)
		signal_handler_signal(obs->signals, signal_obs, &data);
	if (signal_source)
		signal_handler_signal(source->context.signals, signal_source,
				      &data);
}

/* maximum timestamp variance in nanoseconds */
//2秒
#define MAX_TS_VAR 2000000000ULL

static inline bool frame_out_of_bounds(const obs_source_t *source, uint64_t ts)
{
	if (ts < source->last_frame_ts)
		return ((source->last_frame_ts - ts) > MAX_TS_VAR);
	else
		return ((ts - source->last_frame_ts) > MAX_TS_VAR);
}

static inline enum gs_color_format
convert_video_format(enum video_format format, enum video_trc trc)
{
	switch (trc) {
	case VIDEO_TRC_PQ:
	case VIDEO_TRC_HLG:
		return GS_RGBA16F;
	default:
		switch (format) {
		case VIDEO_FORMAT_RGBA:
			return GS_RGBA;
		case VIDEO_FORMAT_BGRA:
		case VIDEO_FORMAT_I40A:
		case VIDEO_FORMAT_I42A:
		case VIDEO_FORMAT_YUVA:
		case VIDEO_FORMAT_AYUV:
			return GS_BGRA;
		case VIDEO_FORMAT_I010:
		case VIDEO_FORMAT_P010:
		case VIDEO_FORMAT_I210:
		case VIDEO_FORMAT_I412:
		case VIDEO_FORMAT_YA2L:
		case VIDEO_FORMAT_P216:
		case VIDEO_FORMAT_P416:
		case VIDEO_FORMAT_V210:
			return GS_RGBA16F;
		default:
			return GS_BGRX;
		}
	}
}

static inline enum gs_color_space convert_video_space(enum video_format format,
						      enum video_trc trc)
{
	enum gs_color_space space = GS_CS_SRGB;
	if (convert_video_format(format, trc) == GS_RGBA16F) {
		switch (trc) {
		case VIDEO_TRC_DEFAULT:
		case VIDEO_TRC_SRGB:
			space = GS_CS_SRGB_16F;
			break;
		case VIDEO_TRC_PQ:
		case VIDEO_TRC_HLG:
			space = GS_CS_709_EXTENDED;
		}
	}

	return space;
}

extern void obs_source_set_texcoords_centered(obs_source_t *source, bool centered);

/*
 1 控制source的主画布及预览画布显示和影藏
 2 控制source的主画布play/stop
 */
extern void obs_source_activate(obs_source_t *source, enum view_type type);
extern void obs_source_deactivate(obs_source_t *source, enum view_type type);


/*
 视频线程tick 回调
 主要处理的任务有:
 1 专场source更新
 2 如果source->defer_update_count有了更新则跟新source->info.update
 3 根据frame准备好当前的纹理及纹理渲染器 
 */
extern void obs_source_video_tick(obs_source_t *source, float seconds);
extern float obs_source_get_target_volume(obs_source_t *source,
					  obs_source_t *target);

extern void obs_source_audio_render(obs_source_t *source, uint32_t mixers,
				    size_t channels, size_t sample_rate,
				    size_t size);

extern void add_alignment(struct vec2 *v, uint32_t align, int cx, int cy);




extern struct obs_source_frame *filter_async_video(obs_source_t *source,
						   struct obs_source_frame *in);
///更新所有纹理的图像数据
extern bool update_async_texture(struct obs_source *source,
				 const struct obs_source_frame *frame,
				 gs_texture_t *tex, gs_texrender_t *texrender);

///更新所有纹理的图像数据
extern bool update_async_textures(struct obs_source *source,
				  const struct obs_source_frame *frame,
				  gs_texture_t *tex[MAX_AV_PLANES],
				  gs_texrender_t *texrender);
///当纹理有变化时会重新创建纹理及渲染器  当纹理有变化时直接返回
extern bool set_async_texture_size(struct obs_source *source,
				   const struct obs_source_frame *frame);
extern void remove_async_frame(obs_source_t *source,
			       struct obs_source_frame *frame);




extern void set_deinterlace_texture_size(obs_source_t *source);
extern void deinterlace_process_last_frame(obs_source_t *source,
					   uint64_t sys_time);
///更新所有纹理的图像数据
extern void deinterlace_update_async_video(obs_source_t *source);
///去隔行渲染 
extern void deinterlace_render(obs_source_t *s);

/* ------------------------------------------------------------------------- */
/* outputs  */

enum delay_msg {
	DELAY_MSG_PACKET,
	DELAY_MSG_START,
	DELAY_MSG_STOP,
};

struct delay_data {
	enum delay_msg msg;
    ///时钟时间 
	uint64_t ts;
	struct encoder_packet packet;
};

typedef void (*encoded_callback_t)(void *data, struct encoder_packet *packet);

struct obs_weak_output {
	struct obs_weak_ref ref;
	struct obs_output *output;
};

/*
 Caption(闭合字幕/隐藏式字幕):
 主要用于听障人士
 除了对话,还包含声音效果、音乐等音频描述
 通常是实时生成的
 可以通过设备开关显示/隐藏
 在OBS中主要用于直播时的实时字幕
 */

#define CAPTION_LINE_CHARS (32)
#define CAPTION_LINE_BYTES (4 * CAPTION_LINE_CHARS)
struct caption_text {
	char text[CAPTION_LINE_BYTES + 1];
	double display_duration;
	struct caption_text *next;
};

struct pause_data {
	pthread_mutex_t mutex;
    ///接收到每一帧视频的ts
    uint64_t last_video_ts;
    ///暂停开始的时间
    uint64_t ts_start;
    ///暂停结束的时间
    uint64_t ts_end;
    ///每次暂停时长的累计时间
    uint64_t ts_offset;
};

extern bool video_pause_check(struct pause_data *pause, uint64_t timestamp);
extern bool audio_pause_check(struct pause_data *pause, struct audio_data *data,
			      size_t sample_rate);
extern void pause_reset(struct pause_data *pause);

/*
 obs_output 也是个双向链表
 表头obs_core_data.在first_output
 
 
 
 obs_output_start ==> output->info->start()
 output->info  =callback=>(output) hookaction
 ==>(+callback 编码后的回调) startEncoder()
 ==>start_raw_video ==添加编码器的输入回调==> video-io->inputs
 
 */

struct obs_output {
	struct obs_context_data context;
	struct obs_output_info info;

	/* indicates ownership of the info.id buffer */
	bool owns_info_id;
    ///当前output是否已经接收到audio video
	bool received_video;
	bool received_audio;
    
	volatile bool data_active;
    ///结束线程是否正在处理 
	volatile bool end_data_capture_thread_active;
    
    ///初始化时 初始化为第一个视频的pts  保证后续的dts pts都是从0开始
	int64_t video_offset;
    ///初始化时 初始化为第一个音频的pts  保证后续的dts pts都是从0开始
	int64_t audio_offsets[MAX_OUTPUT_AUDIO_ENCODERS];
    
    ///指当前已处理的视频数据包中最大的时间戳值
	int64_t highest_audio_ts;
	int64_t highest_video_ts;
    ///停止输出时需要一系列的操作  防止阻塞 开启线程异步处理
	pthread_t end_data_capture_thread;
    ///只有在destory的时候回阻塞线程 保证在销毁之前 end_data_capture_thread线程处理完成
	os_event_t *stopping_event;
	pthread_mutex_t interleaved_mutex;
    ///编码器输出后  音频/视频packet数组
	DARRAY(struct encoder_packet) interleaved_packets;
	int stop_code;
    ///每多长时间重连
	int reconnect_retry_sec;
    ///重连的最大次数
	int reconnect_retry_max;
	int reconnect_retries;
    ///达到重连的时长 (每次发生重连时需要再等待reconnect_retry_cur_msec 一段时间)
	uint32_t reconnect_retry_cur_msec;
	float reconnect_retry_exp;
	pthread_t reconnect_thread;
    ///当发起重新连接时 开启新的线程 此事件会阻塞重连线程reconnect_retry_cur_msec时长 
	os_event_t *reconnect_stop_event;
	volatile bool reconnecting;
	volatile bool reconnect_thread_active;

	uint32_t starting_drawn_count;
    ///滞后
	uint32_t starting_lagged_count;
    ///
	uint32_t starting_frame_count;

	int total_frames;
    ///初始化完毕之后为true
	volatile bool active;
	volatile bool paused;
    ///音视输出
	video_t *video;
	audio_t *audio;
	obs_encoder_t *video_encoder;
	obs_encoder_t *audio_encoders[MAX_OUTPUT_AUDIO_ENCODERS];
	obs_service_t *service;
	size_t mixer_mask;

	struct pause_data pause;
    ///====音频的原始数据 
	struct circlebuf audio_buffer[MAX_AUDIO_MIXES][MAX_AV_PLANES];
    ///当前音频的时间
	uint64_t audio_start_ts;
    ///当前视频的时间
	uint64_t video_start_ts;
    ///plane下音频采样大小  !plane下channels*采样大小
	size_t audio_size;
    /*
     对于非平面格式:
     无论多少声道,planes = 1
     因为所有声道的数据都在一个连续的缓冲区中
     对于平面格式:
     planes 等于声道数
     每个声道的数据存储在独立的平面中
     */
	size_t planes;
	size_t sample_rate;
	size_t total_audio_frames;

	uint32_t scaled_width;
	uint32_t scaled_height;

    ///音频视频变换信息
	bool video_conversion_set;
	bool audio_conversion_set;
	struct video_scale_info video_conversion;
	struct audio_convert_info audio_conversion;

	pthread_mutex_t caption_mutex;
	double caption_timestamp;
	struct caption_text *caption_head;
	struct caption_text *caption_tail;

	struct circlebuf caption_data;

	bool valid;

    ///延迟的纳秒数delay_sec*1000000000* 超出这个时长才会处理
	uint64_t active_delay_ns;
    ///当延迟处理过后 回调给正常流程
	encoded_callback_t delay_callback;
    ///开启延迟时 数据存储在delay_data中 超出时长才会pop
	struct circlebuf delay_data; /* struct delay_data */
	pthread_mutex_t delay_mutex;
    ///延迟的秒数
	uint32_t delay_sec;
	uint32_t delay_flags;
	uint32_t delay_cur_flags;
    ///当开启延迟 并且当断开需要重连时 此时+1  此时再次重启怎destory线程不会开启 保证延迟数据还在
	volatile long delay_restart_refs;
    ///延迟功能有没有打开
	volatile bool delay_active;
    ///是否开启延迟捕获 开启时会清空缓冲区 重新缓存delay_sec时长的数据
	volatile bool delay_capturing;

	char *last_error_message;

	float audio_data[MAX_AUDIO_CHANNELS][AUDIO_OUTPUT_FRAMES];
};

static inline void do_output_signal(struct obs_output *output,
				    const char *signal)
{
	struct calldata params = {0};
	calldata_set_ptr(&params, "output", output);
	signal_handler_signal(output->context.signals, signal, &params);
	calldata_free(&params);
}

extern void process_delay(void *data, struct encoder_packet *packet);
extern void obs_output_cleanup_delay(obs_output_t *output);
extern bool obs_output_delay_start(obs_output_t *output);
extern void obs_output_delay_stop(obs_output_t *output);
extern bool obs_output_actual_start(obs_output_t *output);
extern void obs_output_actual_stop(obs_output_t *output, bool force,
				   uint64_t ts);

extern const struct obs_output_info *find_output(const char *id);

extern void obs_output_remove_encoder(struct obs_output *output,
				      struct obs_encoder *encoder);

extern void
obs_encoder_packet_create_instance(struct encoder_packet *dst,
				   const struct encoder_packet *src);
void obs_output_destroy(obs_output_t *output);

/* ------------------------------------------------------------------------- */
/* encoders  */

struct obs_weak_encoder {
	struct obs_weak_ref ref;
	struct obs_encoder *encoder;
};

struct encoder_callback {
	bool sent_first_packet;
	void (*new_packet)(void *param, struct encoder_packet *packet);
	void *param;
};
///跟source一样 自身双链表
struct obs_encoder {
	struct obs_context_data context;
	struct obs_encoder_info info;

	/* allows re-routing to another encoder */
	struct obs_encoder_info orig_info;

	pthread_mutex_t init_mutex;

	uint32_t samplerate;
    ///数据的平面数(如果是交错的则就一个平面 )
	size_t planes;
    ///1个采样(planner下每个平面代表一个块 interleaved下就一块 ) (1个采样双声道16位采样双声道下   planer时8  在Interleaved时16 )  一般在内存层面使用
	size_t blocksize;
    ///每一帧的采样数（1024） out_frames_per_packet
	size_t framesize;
    ///一个帧下每个块的大小   blocksize*framesize
	size_t framesize_bytes;
    
	size_t mixer_idx;
	uint32_t scaled_width;
	uint32_t scaled_height;
	enum video_format preferred_format;

	volatile bool active;
    ///只是一个暂停标记   没有其他逻辑
	volatile bool paused;
	bool initialized;

	/* indicates ownership of the info.id buffer */
	bool owns_info_id;

	uint32_t timebase_num;
	uint32_t timebase_den;

	int64_t cur_pts;
    ///外部给编码器 输入数据的缓冲区
	struct circlebuf audio_input_buffer[MAX_AV_PLANES];
    ///即将要编码的缓冲区
	uint8_t *audio_output_buffer[MAX_AV_PLANES];

	/* if a video encoder is paired with an audio encoder, make it start
	 * up at the specific timestamp.  if this is the audio encoder,
	 * wait_for_video makes it wait until it's ready to sync up with
	 * video */
	bool wait_for_video;
    ///首次接收到音频数据 
	bool first_received;
    ///在同一路输出时 同时会有音频视频的编码器  如果此时是音频编码器咋表示视频编码器
	struct obs_encoder *paired_encoder;
	int64_t offset_usec;
    ///第一个音频时间戳
	uint64_t first_raw_ts;
    ///第一帧的编码时间
	uint64_t start_ts;

	pthread_mutex_t outputs_mutex;
	DARRAY(obs_output_t *) outputs;

	bool destroy_on_stop;

	/* stores the video/audio media output pointer.  video_t *or audio_t **/
	void *media;

    ///编码后的回调
	pthread_mutex_t callbacks_mutex;
	DARRAY(struct encoder_callback) callbacks;

	struct pause_data pause;

	const char *profile_encoder_encode_name;
	char *last_error_message;

	/* reconfigure encoder at next possible opportunity */
    ///重新更新编码器的配置
	bool reconfigure_requested;
};

extern struct obs_encoder_info *find_encoder(const char *id);

extern bool obs_encoder_initialize(obs_encoder_t *encoder);
extern void obs_encoder_shutdown(obs_encoder_t *encoder);

extern void obs_encoder_start(obs_encoder_t *encoder,
			      void (*new_packet)(void *param,
						 struct encoder_packet *packet),
			      void *param);
extern void obs_encoder_stop(obs_encoder_t *encoder,
			     void (*new_packet)(void *param,
						struct encoder_packet *packet),
			     void *param);

extern void obs_encoder_add_output(struct obs_encoder *encoder,
				   struct obs_output *output);
extern void obs_encoder_remove_output(struct obs_encoder *encoder,
				      struct obs_output *output);

extern bool start_gpu_encode(obs_encoder_t *encoder);
extern void stop_gpu_encode(obs_encoder_t *encoder);

extern bool do_encode(struct obs_encoder *encoder, struct encoder_frame *frame);
extern void send_off_encoder_packet(obs_encoder_t *encoder, bool success,
				    bool received, struct encoder_packet *pkt);

void obs_encoder_destroy(obs_encoder_t *encoder);

/* ------------------------------------------------------------------------- */
/* services */

struct obs_weak_service {
	struct obs_weak_ref ref;
	struct obs_service *service;
};
///只参与获取信息  没有其它逻辑
struct obs_service {
	struct obs_context_data context;
	struct obs_service_info info;

	/* indicates ownership of the info.id buffer */
	bool owns_info_id;

	bool active;
	bool destroy;
	struct obs_output *output;
};

extern const struct obs_service_info *find_service(const char *id);

extern void obs_service_activate(struct obs_service *service);
extern void obs_service_deactivate(struct obs_service *service, bool remove);
extern bool obs_service_initialize(struct obs_service *service,
				   struct obs_output *output);

void obs_service_destroy(obs_service_t *service);

void obs_output_remove_encoder_internal(struct obs_output *output,
					struct obs_encoder *encoder);
