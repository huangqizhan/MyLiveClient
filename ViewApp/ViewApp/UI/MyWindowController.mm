//
//  MyWindowController.m
//  ViewApp
//
//  Created by santian_mac on 2024/8/22.
//  

#import "MyWindowController.h"
#include <api/ObsMain.h>
#include <api/ObsApp.hpp>
#include <util/darray.h>

@interface MyWindowController ()<NSWindowDelegate>

@end

@implementation MyWindowController

- (void)windowDidLoad {
    [super windowDidLoad];
    [self.window setMaxSize:NSMakeSize(1000,600)];
    [self.window setMinSize:NSMakeSize(1000,600)];
    
    self.window.delegate = self;

    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
}
- (void)windowWillClose:(NSNotification *)notification {
    ObsMain::Instance()->shutDown();
}
@end
