//
//  ObsApp.cpp
//  ViewApp
//
//  Created by santian_mac on 2024/8/28.
//

#include "ObsApp.hpp"
#include <signal.h>


ObsApp *obsApp = nullptr;



ObsApp *ObsApp::Instance(){
    if (!obsApp) {
        obsApp = new ObsApp();
    }
    return obsApp;
}
void ObsApp::HandelSignal(){
    
    //忽略SIGPIPE信号
    signal(SIGPIPE, SIG_IGN);
    //让其他线程也忽略此信号
    sigset_t sigpipe_mask;
    sigemptyset(&sigpipe_mask);
    sigaddset(&sigpipe_mask, SIGPIPE);
    sigset_t saved_mask;
    if (pthread_sigmask(SIG_BLOCK, &sigpipe_mask, &saved_mask) == -1) {
        perror("pthread_sigmask");
    }
    
    ///处理SIGINT信号
    struct sigaction sig_handler;
    sig_handler.sa_handler = ObsApp::SigIntSignalHandler;
    sigemptyset(&sig_handler.sa_mask);
    sig_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_handler, NULL);
    
    
}
void ObsApp::SigIntSignalHandler(int sig){
    printf("app close \n");
}
