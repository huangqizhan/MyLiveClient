//
//  MyViewController.m
//  ViewApp
//
//  Created by santian_mac on 2024/8/22.
//

#import "MyViewController.h"
#import "MyViewController+UI.h"
#import <api/ObsBasic.h>
#import <api/ObsMain.h>
#import "MyDisplayView.h"
#include <api/ObsUtils.h>


@interface MyViewController ()

- (void)OnAddScene:(OBSScene)source;
- (void)OnRemoveScene:(OBSScene)source;
- (void)OnReorderScene;

- (void)OnActivateAudioSource:(OBSSource)source;
- (void)OnDeactivateAudioSource:(OBSSource )source;
- (void)OnRenameSources:(OBSSource )source
                oldName:(const char* )oldName
                newName:(const char* )newName;

//重新加载sceneitem
- (void)OnReloadSceneItemList;
- (void)OnSceneItemSelectChanged:(int)index;
//推流回调接口
- (void)OnStreamStopping;
- (void)OnStreamingStart;
- (void)OnStreamingStop:(int)code err: (const char*)error;
//录制回调接口
- (void)OnRecordingStart;
- (void)OnRecordStopping;
- (void)OnRecordingStop:(int)code;
//显示菜单
- (void)OnMenu:(window_handle_t)handle point:(const ObsPoint& )point;
//设置里重置video大小
- (void)OnVideoReset;

@end


void _OnAddScene(OBSScene source,void *opea){
    MyViewController *vc = (__bridge MyViewController *)opea;
    [vc OnAddScene:source];
}
void _OnRemoveScene(OBSScene source,void *opea){
    MyViewController *vc = (__bridge MyViewController *)opea;
    [vc OnRemoveScene:source];
}
void _OnReorderScene(void *opea){
    MyViewController *vc = (__bridge MyViewController *)opea;
    [vc OnReorderScene];
}
void _OnActivateAudioSource(OBSSource source,void *opea){
    MyViewController *vc = (__bridge MyViewController *)opea;
    [vc OnActivateAudioSource:source];
}
void _OnDeactivateAudioSource(OBSSource source,void *opea){
    MyViewController *vc = (__bridge MyViewController *)opea;
    [vc OnDeactivateAudioSource:source];
}
void _OnRenameSources(OBSSource source,
                      const char* oldName,const char* newName,void *opea){
    MyViewController *vc = (__bridge MyViewController *)opea;
    [vc OnRenameSources:source oldName:oldName newName:newName];
}
void _OnReloadSceneItemList(void *opea){
    MyViewController *vc = (__bridge MyViewController *)opea;
    [vc OnReloadSceneItemList];
}
void _OnSceneItemSelectChanged(int index,void *opea){
    MyViewController *vc = (__bridge MyViewController *)opea;
    [vc OnSceneItemSelectChanged:index];
}
void _OnStreamStopping(void *opea){
    MyViewController *vc = (__bridge MyViewController *)opea;
    [vc OnStreamStopping];
}
void _OnStreamingStart(void *opea){
    MyViewController *vc = (__bridge MyViewController *)opea;
    [vc OnStreamingStart];
}

void _OnStreamingStop(int code, const char* error,void *opea){
    MyViewController *vc = (__bridge MyViewController *)opea;
    [vc OnStreamingStop:code err:error];
};
//录制回调接口
void _OnRecordingStart(void *opea) {
    MyViewController *vc = (__bridge MyViewController *)opea;
    [vc OnRecordingStart];
};
void _OnRecordStopping(void *opea) {
    MyViewController *vc = (__bridge MyViewController *)opea;
    [vc OnRecordStopping];
};
void _OnRecordingStop(int code,void *opea){
    MyViewController *vc = (__bridge MyViewController *)opea;
    [vc OnRecordingStop:code];
};
//显示菜单
void _OnMenu(window_handle_t handle,const ObsPoint& point,void *opea){
    MyViewController *vc = (__bridge MyViewController *)opea;
    [vc OnMenu:handle point:point];
};
//设置里重置video大小
void _OnVideoReset(void *opea){
    MyViewController *vc = (__bridge MyViewController *)opea;
    [vc OnVideoReset];
};

class  MainOberver : public ObsObserver{
public:
    MainOberver(void *opea):mopea(opea){}
    ~MainOberver(){}
    virtual void OnAddScene(OBSScene source) override{
        _OnAddScene(source, mopea);
    }
    virtual void OnRemoveScene(OBSScene source) override {
        _OnRemoveScene(source, mopea);
    };
    virtual void OnReorderScene() override{
        _OnReorderScene(mopea);
    };
    
    virtual void OnActivateAudioSource(OBSSource source) override{
        _OnActivateAudioSource(source,mopea);
    };
    
    virtual void OnDeactivateAudioSource(OBSSource source) override{
        _OnDeactivateAudioSource(source,mopea);
    };
    
    virtual void OnRenameSources(OBSSource source,
                                 const char* oldName,const char* newName) override{
        _OnRenameSources(source,oldName,newName,mopea);
    };
    
    //重新加载sceneitem
    virtual void OnReloadSceneItemList() override{
        _OnReloadSceneItemList(mopea);
    };
    virtual void OnSceneItemSelectChanged(int index) override {
        _OnSceneItemSelectChanged(index,mopea);
    };
    //推流回调接口
    virtual void OnStreamStopping() override{
        _OnStreamStopping(mopea);
    };
    virtual void OnStreamingStart() override{
        _OnStreamingStart(mopea);
    };
    virtual void OnStreamingStop(int code, const char* error) override{
        _OnStreamingStop(code,error,mopea);
    };
    //录制回调接口
    virtual void OnRecordingStart() override{
        _OnRecordingStart(mopea);
    };
    virtual void OnRecordStopping() override{
        _OnRecordStopping(mopea);
    };
    virtual void OnRecordingStop(int code) override{
        _OnRecordingStop(code,mopea);
    };
    
    //显示菜单
    virtual void OnMenu(window_handle_t handle,const ObsPoint& point) override{
        _OnMenu(handle,point,mopea);
    };
    
    //设置里重置video大小
    virtual void OnVideoReset() override{
        _OnVideoReset(mopea);
    };
private:
    void *mopea;
};
@interface MyViewController (){
    ObsMain *_m_obs;
    MainOberver *_mobserver;
}
@property (nonatomic,strong) NSMutableArray *sceneNames;
@property (nonatomic,strong) NSMutableArray *mediaNames;

@end
@implementation MyViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    _m_obs = ObsMain::Instance();
    void * pself = (__bridge void *)self;
    _mobserver = new MainOberver(pself);
    _m_obs->SetObserver(_mobserver);
    [self initSceneListUI];
    [self initSourceListUI];
    
    self.sceneNames = [NSMutableArray array];
    [self.sceneNames addObject:@"s1"];
    [self.sceneNames addObject:@"s2"];
    
    self.mediaNames = [NSMutableArray array];
    [self.mediaNames addObject:@"awesomeface"];
    [self.mediaNames addObject:@"container2"];

    /*
     -I/Users/me/webrtc/src
     -I/Users/me/webrtc/src/third_party/abseil-cpp
     -I/Users/me/webrtc/src/out/mac/gen
     */
}
- (void)OnAddScene:(OBSScene)source{
    [self addScene:source];
}
- (void)OnRemoveScene:(OBSScene)source{
    NSLog(@"%s",__func__);
}
- (void)OnReorderScene{
    NSLog(@"%s",__func__);
}
- (void)OnActivateAudioSource:(OBSSource)source{
    NSLog(@"%s",__func__);
}
- (void)OnDeactivateAudioSource:(OBSSource )source {
    NSLog(@"%s",__func__);
}
- (void)OnRenameSources:(OBSSource )source
                oldName:(const char* )oldName
                newName:(const char* )newName{
    NSLog(@"%s",__func__);
}
//重新加载
- (void)OnReloadSceneItemList{
    for (NSView *sv in self.sourceStackView.views) {
        [self.sourceStackView removeView:sv];
    }
    ObsSceneItemList& itemList = _m_obs->sceneItemList();
    for (int i = 0; i < itemList.Count(); ++i){
        [self addSceneItem:itemList.Get(i)];
    }
}
- (void)OnSceneItemSelectChanged:(int)index{
    NSLog(@"%s",__func__);
}
//推流回调接口
- (void)OnStreamStopping{
    NSLog(@"%s",__func__);
}
- (void)OnStreamingStart{
    NSLog(@"%s",__func__);
}
- (void)OnStreamingStop:(int)code err: (const char*)error{
    NSLog(@"%s",__func__);
}
//录制回调接口
- (void)OnRecordingStart{
    NSLog(@"%s",__func__);
}
- (void)OnRecordStopping{
    NSLog(@"%s",__func__);
}
- (void)OnRecordingStop:(int)code{
    NSLog(@"%s",__func__);
}
//显示菜单
- (void)OnMenu:(window_handle_t)handle point:(const ObsPoint& )point{
    NSLog(@"%s",__func__);
}
//设置里重置video大小
- (void)OnVideoReset{
    NSLog(@"%s",__func__);
}


#pragma mark ----- private
- (void)addScene:(OBSScene)scene{
    OBSSource source = obs_scene_get_source(scene);
    const char*sname = obs_source_get_name(source)?:"";
    NSButton *button = [NSButton buttonWithTitle:[NSString stringWithUTF8String:sname]
                                          target:self
                                          action:@selector(sceneButtonClicked:)];
    [self.sceneStackView  addView:button inGravity:(NSStackViewGravityTrailing)];
}
- (void)addSceneItem:(OBSSceneItem)sceneitem{
    
    const char*itemName = ObsSceneItemList::itemName(sceneitem);
    bool bVisible = ObsSceneItemList::itemVisible(sceneitem);
    bool bSelected = ObsSceneItemList::itemSelected(sceneitem);
//    obs_sceneitem_set_alignment(sceneitem, OBS_ALIGN_RIGHT|OBS_ALIGN_TOP);
//    obs_sceneitem_set_bounds_type(sceneitem, OBS_BOUNDS_SCALE_INNER);
    NSButton *button = [NSButton buttonWithTitle:[NSString stringWithUTF8String:itemName]
                                          target:self
                                          action:@selector(buttonClicked:)];
    [self.sourceStackView addView:button inGravity:(NSStackViewGravityTrailing)];
}

#pragma mark actions
- (void)sceneButtonClicked:(NSButton *)sender{
    const char *cname = sender.title.UTF8String;
    OBSScene scene = _m_obs->FindScene(cname);
    _m_obs->SetCurrentScene(scene);
}
- (void)buttonClicked:(NSButton *)sender{
    NSLog(@"buttonClicked");
//    _mobserver->OnVideoReset();
}
- (IBAction)startBtnAction:(NSButton *)sender {
    NSString *name = @"engclpi";
    NSString *path = [[NSBundle mainBundle] pathForResource:name ofType:@"mp4"];
//    NSString *path = [[NSBundle mainBundle] pathForResource:@"engclpi" ofType:@"mp4"];
    if(path.length == 0){
        blog(LOG_ERROR, "file path empty!!! ");;
        return;
    }
    std::string sname = "video";
    std::string url = path.UTF8String;
    ObsMain::VideoData vdata = {
      .name = sname,
      .url = url,
      .isFile = true,
    };
    _m_obs->AddVideo(&vdata);
}
- (IBAction)mediaBtnAction:(NSButton *)sender {
    if(self.mediaNames.count <= 0) return;
    NSString *name = self.mediaNames.firstObject;
    NSString *path = [[NSBundle mainBundle] pathForResource:name ofType:@"png"];
//    NSString *path = [[NSBundle mainBundle] pathForResource:@"engclpi" ofType:@"mp4"];
    if(path.length == 0){
        blog(LOG_ERROR, "file path empty!!! ");;
        return;
    }
    _m_obs->AddImage(path.UTF8String, 120);
    [self.mediaNames removeObject:name];
//   ObsMain::VideoData vdata;
//    vdata.name = "video";
//    vdata.url = path.UTF8String;
//    vdata.isFile = true;
//    vdata.isActiveReplay = true;
//    vdata.isLoop = true;
//    _m_obs->AddVideo(&vdata);
}
- (IBAction)cameraBtnAction:(id)sender {
//    NSButton *button = [NSButton buttonWithTitle:@"camera"
//                                          target:self
//                                          action:@selector(buttonClicked:)];
//    [self.sourceStackView  addView:button inGravity:(NSStackViewGravityTrailing)];
}
- (IBAction)windowBtnAction:(NSButton *)sender {
//    NSButton *button = [NSButton buttonWithTitle:@"window"
//                                          target:self
//                                          action:@selector(buttonClicked:)];
//    [self.sourceStackView  addView:button inGravity:(NSStackViewGravityTrailing)];
}
- (IBAction)screenBtnAction:(NSButton *)sender {
    if(self.sceneNames.count <= 0) return;
    NSString *name = self.sceneNames.firstObject;
    const char *cname = name.UTF8String;
    _m_obs->AddScene(cname, true);
    [self.sceneNames removeObject:name];
}


@end




