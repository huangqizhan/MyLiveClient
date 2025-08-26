//
//  ObsApp.hpp
//  ViewApp
//
//  Created by santian_mac on 2024/8/28.
//

#ifndef ObsApp_hpp
#define ObsApp_hpp

#include <stdio.h>


class ObsApp {
    
public:
    static void HandelSignal();
    static ObsApp *Instance();
    static void  SigIntSignalHandler(int);
    
private:
    
};





#endif /* ObsApp_hpp */
