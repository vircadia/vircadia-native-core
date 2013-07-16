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
//          DOMAIN_HOSTNAME or DOMAIN_IP strings in the file NodeList.cpp
//
//
//  Welcome Aboard!
//

#include "Application.h"

#include <QDebug>

int main(int argc, const char * argv[]) {
    timeval startup_time;
    gettimeofday(&startup_time, NULL);
    
    Application app(argc, const_cast<char**>(argv), startup_time);
    qDebug( "Created QT Application." );
    int exitCode = app.exec();
    qDebug("Normal exit.");
    return exitCode;
}   
