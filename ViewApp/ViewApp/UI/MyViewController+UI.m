//
//  MyWindowController+UI.m
//  ViewApp
//
//  Created by santian_mac on 2024/8/29.
//

#import "MyViewController+UI.h"

@implementation MyViewController (UI)


- (void)initSceneListUI{
    // 创建 NSStackView
    self.sceneStackView = [[NSStackView alloc] init];
    
    self.sceneStackView.orientation = NSUserInterfaceLayoutOrientationHorizontal;
//    self.sceneStackView.alignment = NSLayoutAttributeLeading;
    self.sceneStackView.distribution = NSStackViewDistributionFillEqually;
    self.sceneStackView.spacing = 10;
//    self.sceneStackView.wantsLayer = 1;
//    self.sceneStackView.layer.backgroundColor = NSColor.redColor.CGColor;

    self.sceneStackView.translatesAutoresizingMaskIntoConstraints = NO;
    
    [self.topView addSubview:self.sceneStackView];
    [NSLayoutConstraint activateConstraints:@[
        [self.sceneStackView.topAnchor constraintEqualToAnchor:self.topView.topAnchor],
        [self.sceneStackView.bottomAnchor constraintEqualToAnchor:self.topView.bottomAnchor],
        [self.sceneStackView.leadingAnchor constraintEqualToAnchor:self.topView.leadingAnchor],
//        [self.sceneStackView.trailingAnchor constraintEqualToAnchor:self.topView.trailingAnchor],
    ]];
}
- (void)initSourceListUI{
    // 创建 NSStackView
    self.sourceStackView = [[NSStackView alloc] init];
    
    self.sourceStackView.orientation = NSUserInterfaceLayoutOrientationVertical;
    self.sourceStackView.alignment = NSLayoutAttributeLeading;
    self.sourceStackView.distribution = NSStackViewDistributionFill;
    self.sourceStackView.spacing = 10;

    self.sourceStackView.translatesAutoresizingMaskIntoConstraints = NO;
    // 创建并添加按钮
//    for (int i = 1; i <= 8; i++) {
//        NSButton *button = [NSButton buttonWithTitle:[NSString stringWithFormat:@"Button %d", i]
//                                              target:self
//                                              action:@selector(buttonClicked:)];
//        [self.courceStackView addView:button inGravity:(NSStackViewGravityTop)];
//    }
    [self.leftView addSubview:self.sourceStackView];
    [NSLayoutConstraint activateConstraints:@[
        [self.sourceStackView.topAnchor constraintEqualToAnchor:self.leftView.topAnchor constant:10],
        [self.sourceStackView.leadingAnchor constraintEqualToAnchor:self.leftView.leadingAnchor],
        [self.sourceStackView.trailingAnchor constraintEqualToAnchor:self.leftView.trailingAnchor],
    ]];
}

@end
