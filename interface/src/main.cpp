//  
//  Interface
//
//  Allows you to connect to and see/hear the shared 3D space.
//  Optionally uses serialUSB connection to get gyro data for head movement.
//  Optionally gets UDP stream from transmitter to animate controller/hand.
//  
//  Usage:  The interface client first attempts to contact a domain server to
//          discover the appropriate audio, voxel, and avatar servers to contact.
//          Right now, the default domain server is "highfidelity.below92.com"
//          You can change the domain server to use your own by editing the
//          DOMAIN_HOSTNAME or DOMAIN_IP strings in the file AgentList.cpp
//
//
//  Welcome Aboard!
//

#include "Application.h"
#include "Log.h"

#include <OctalCode.h>

int main(int argc, const char * argv[]) {

    unsigned char test1[2] = {2, 0xE0 };
    unsigned char test2[2] = {2, 0xFC };

    unsigned char* result1 = chopOctalCode((unsigned char*)&test1, 1);
    
    printOctalCode((unsigned char*)&test1);
    printOctalCode(result1);

    unsigned char* result2 = chopOctalCode((unsigned char*)&test2, 1);
    printOctalCode((unsigned char*)&test2);
    printOctalCode(result2);
    
    timeval startup_time;
    gettimeofday(&startup_time, NULL);
    
    Application app(argc, const_cast<char**>(argv), startup_time);
    printLog( "Created QT Application.\n" );
    int exitCode = app.exec();
    printLog("Normal exit.\n");
    return exitCode;
}   
