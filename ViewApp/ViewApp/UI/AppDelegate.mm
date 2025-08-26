//
//  AppDelegate.m
//  ViewApp
//
//  Created by santian_mac on 2024/8/22.
//

#import "AppDelegate.h"
#import "MyWindowController.h"
#import "MyViewController.h"
#include <api/ObsMain.h>
#include <api/ObsApp.hpp>
#include <util/darray.h>


@interface AppDelegate ()
@property(nonatomic,strong) MyWindowController *myWindow;


@end

@implementation AppDelegate


- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {

//    DARRAY(int) iarr;
//    da_init(iarr);
//    int v1 = 12;
//    da_push_back(iarr, &v1);
//    da_push_back(iarr, &v1);
//    da_pop_back(iarr);
//    da_free(iarr);
    
    ObsApp::HandelSignal();
    
    ObsMain *obsMain = ObsMain::Instance();
    
    if (!obsMain->MakeUserDirs()){
        blog(LOG_ERROR, "Failed to create required user directories");
    }

    if (!obsMain->InitGlobalConfig()){
        blog(LOG_ERROR, "Failed to initialize global config");
    }

    if (!obsMain->MakeUserProfileDirs()){
        blog(LOG_ERROR, "Failed to create profile directories");
    }

    if (!obsMain->InitObs()){
        blog(LOG_ERROR, "Failed to init obs");
    }
    
    self.myWindow = [[MyWindowController alloc] initWithWindowNibName:@"MyWindowController"];
    self.myWindow.window.releasedWhenClosed = YES;
    self.myWindow.window.contentViewController = [[MyViewController alloc] initWithNibName:@"MyViewController" bundle:[NSBundle mainBundle]];
    [self.myWindow.window center];
    [self.myWindow.window orderFront:nil];
    
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Insert code here to tear down your application
}


- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app {
    return YES;
}


@end
