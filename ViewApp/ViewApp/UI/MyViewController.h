//
//  MyViewController.h
//  ViewApp
//
//  Created by santian_mac on 2024/8/22.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN
@class MyDisplayView;

@interface MyViewController : NSViewController
@property (nonatomic,strong) NSStackView *sourceStackView;
@property (nonatomic,strong) NSStackView *sceneStackView;
@property (weak) IBOutlet MyDisplayView *displayView;

@property (weak) IBOutlet NSView *topView;
@property (weak) IBOutlet NSView *leftView;
@property (weak) IBOutlet NSView *bottomView;

@property (weak) IBOutlet NSButton *screenBtn;
@property (weak) IBOutlet NSButton *windowBtn;
@property (weak) IBOutlet NSButton *cameraBtn;
@property (weak) IBOutlet NSButton *mediaBtn;
@property (weak) IBOutlet NSButton *startBtn;



@end

NS_ASSUME_NONNULL_END
