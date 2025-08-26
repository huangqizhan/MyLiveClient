//
//  MyDisplayView.m
//  ViewApp
//
//  Created by santian_mac on 2024/8/29.
//

#import "MyDisplayView.h"
#include <api/ObsWindow.h>

@interface MyDisplayView ()

@end


//ObsRect _GetClientRect(window_handle_t opea){
//
//    MyDisplayView *dview = (__bridge MyDisplayView *)opea;
//    NSRect rect = dview.frame;
//    return {static_cast<int>(rect.origin.x),static_cast<int>(rect.origin.y),static_cast<int>(rect.size.width),static_cast<int>(rect.size.height)};
//}
void _SetWindowPos(const ObsRect& rc,window_handle_t opea){
    dispatch_async(dispatch_get_main_queue(), ^{
        MyDisplayView *dview = (__bridge MyDisplayView *)opea;
        dview.frame = NSMakeRect(rc.x, rc.y, rc.width, rc.height);
    });
}
void _SetVisible(bool visible,window_handle_t opea){
    dispatch_async(dispatch_get_main_queue(), ^{
        MyDisplayView *dview = (__bridge MyDisplayView *)opea;
        dview.hidden = !visible;
    });
}


class CDisplayView : public ObsWindow {
            
public:
    CDisplayView(void *_display):m_hWnd(_display){}
    ~CDisplayView(){}

    //obs接口实现
    ObsSize GetClientSize() override{
        if (m_hWnd){
//            ObsRect rect = _GetClientRect(m_hWnd);
//            return ObsSize(rect.width, rect.height);
            return ObsSize(m_display_rect.width, m_display_rect.height);
        }
        return ObsSize();
    };
    window_handle_t CreateWnd(window_handle_t parent, const ObsRect& pos) override{
        return m_hWnd;
    }
    void SetWindowPos(const ObsRect& rc) override{
        _SetWindowPos(rc,m_hWnd);
    }
    window_handle_t GetWndHandle() override{
        return m_hWnd;
    }
    bool CheckKeyState(unsigned int state) override{
        return false;
    }
    void SetVisible(bool visible) override{
        _SetVisible(visible,m_hWnd);
    }
    void SetDisplayRect(ObsRect rect){
        m_display_rect = rect;
    }
    
private:
    window_handle_t m_hWnd;
    ObsRect m_display_rect;
};



@implementation MyDisplayView{
    NSView *_displayView;
    CDisplayView *_obs_display;
}
- (void)awakeFromNib{
    [super awakeFromNib];
    [self setupDisplayView];
    window_handle_t pv = (__bridge window_handle_t)_displayView;
    _obs_display = new CDisplayView(pv);
    NSRect rect = _displayView.frame;
    _obs_display->SetDisplayRect({static_cast<int>(rect.origin.x),static_cast<int>(rect.origin.y),static_cast<int>(rect.size.width),static_cast<int>(rect.size.height)});
    _obs_display->CreateDisplay();
}
- (void)setupDisplayView{
    _displayView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, self.frame.size.width, self.frame.size.height)];
    _displayView.wantsLayer = 1;
    _displayView.layer.backgroundColor = NSColor.blackColor.CGColor;
    [self addSubview:_displayView];
    [NSLayoutConstraint activateConstraints:@[
        [_displayView.heightAnchor constraintEqualToAnchor:self.heightAnchor],
        [_displayView.widthAnchor constraintEqualToAnchor:self.widthAnchor],
        [_displayView.centerYAnchor constraintEqualToAnchor:self.centerYAnchor],
        [_displayView.centerYAnchor constraintEqualToAnchor:self.centerYAnchor],
    ]];
}

- (void)mouseDown:(NSEvent *)event{
    CGPoint wp = [event locationInWindow];
    CGPoint mouseLocation = [self convertPoint:wp fromView:nil];
    ObsMouseEvent mevent;
    mevent.button = LeftButton;
    mevent.x = (int)mouseLocation.x;
    mevent.y = (int)(_displayView.frame.size.height - mouseLocation.y);
    _obs_display->OnMousePressEvent(&mevent);
}
- (void)mouseUp:(NSEvent *)event{
    CGPoint wp = [event locationInWindow];
    CGPoint mouseLocation = [self convertPoint:wp fromView:nil];
    ObsMouseEvent mevent;
    mevent.button = LeftButton;
    mevent.x = (int)mouseLocation.x;
    mevent.y = (int)(_displayView.frame.size.height - mouseLocation.y);
    _obs_display->OnMouseReleaseEvent(&mevent);
}
- (void)mouseDragged:(NSEvent *)event{
    CGPoint wp = [event locationInWindow];
    CGPoint mouseLocation = [self convertPoint:wp fromView:nil];
    ObsMouseEvent mevent;
    mevent.button = LeftButton;
    mevent.x = (int)mouseLocation.x;
    mevent.y = (int)(_displayView.frame.size.height - mouseLocation.y);
    _obs_display->OnMouseMoveEvent(&mevent);
//    NSLog(@"x = %d y = %d ",mevent.x,mevent.y);
    
}

@end
