//
//  main.cpp
//  assignment-client
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <Logging.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <VoxelServer.h>

#include "Agent.h"
#include "Assignment.h"
#include "AssignmentClient.h"
#include "audio/AudioMixer.h"
#include "avatars/AvatarMixer.h"

const char PARENT_TARGET_NAME[] = "assignment-client-monitor";

pid_t* childForks = NULL;
HifiSockAddr customAssignmentSocket;
int numForks = 0;
Assignment::Type overiddenAssignmentType = Assignment::AllTypes;
const char* assignmentPool = NULL;

int argc = 0;
char** argv = NULL;

int childClient() {
    AssignmentClient client(::argc, ::argv, ::overiddenAssignmentType, customAssignmentSocket, ::assignmentPool);
    return client.exec();
}

void sigchldHandler(int sig) {
    pid_t processID;
    int status;
    
    while ((processID = waitpid(-1, &status, WNOHANG)) != -1) {
        if (processID == 0) {
            // there are no more children to process, break out of here
            break;
        }
        
        int newForkProcessID = 0;
        
        // find the dead process in the array of child forks
        for (int i = 0; i < ::numForks; i++) {
            if (::childForks[i] == processID) {
                
                newForkProcessID = fork();
                if (newForkProcessID == 0) {
                    // this is the child, call childClient
                    childClient();
                    
                    // break out so we don't fork bomb
                    break;
                } else {
                    // this is the parent, replace the dead process with the new one
                    ::childForks[i] = newForkProcessID;
                   
                    qDebug("Replaced dead %d with new fork %d\n", processID, newForkProcessID);
                    
                    break;
                }
            }
        }
    }
    
}

void parentMonitor() {
    
    struct sigaction sa;
    
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchldHandler;
    
    sigaction(SIGCHLD, &sa, NULL);
    
    pid_t childID = 0;
    
    // don't bail until all children have finished
    while ((childID = waitpid(-1, NULL, 0))) {
        if (errno == ECHILD) {
            break;
        }
    }
    
    // delete the array of pid_t holding the forked process IDs
    delete[] ::childForks;
}

int main(int argc, char* argv[]) {
    ::argc = argc;
    ::argv = argv;
    
    setvbuf(stdout, NULL, _IOLBF, 0);
    
    // use the verbose message handler in Logging
    qInstallMessageHandler(Logging::verboseMessageHandler);
    
    // start the Logging class with the parent's target name
    Logging::setTargetName(PARENT_TARGET_NAME);
    
    const char CUSTOM_ASSIGNMENT_SERVER_HOSTNAME_OPTION[] = "-a";
    const char CUSTOM_ASSIGNMENT_SERVER_PORT_OPTION[] = "-p";
    
    // grab the overriden assignment-server hostname from argv, if it exists
    const char* customAssignmentServerHostname = getCmdOption(argc, (const char**)argv, CUSTOM_ASSIGNMENT_SERVER_HOSTNAME_OPTION);
    const char* customAssignmentServerPortString = getCmdOption(argc,(const char**)argv, CUSTOM_ASSIGNMENT_SERVER_PORT_OPTION);
    
    if (customAssignmentServerHostname || customAssignmentServerPortString) {
        
        // set the custom port or default if it wasn't passed
        unsigned short assignmentServerPort = customAssignmentServerPortString
            ? atoi(customAssignmentServerPortString) : DEFAULT_DOMAIN_SERVER_PORT;
        
        // set the custom hostname or default if it wasn't passed
        if (!customAssignmentServerHostname) {
            customAssignmentServerHostname = DEFAULT_ASSIGNMENT_SERVER_HOSTNAME;
        }
        
        ::customAssignmentSocket = HifiSockAddr(customAssignmentServerHostname, assignmentServerPort);
    }
    
    const char ASSIGNMENT_TYPE_OVVERIDE_OPTION[] = "-t";
    const char* assignmentTypeString = getCmdOption(argc, (const char**)argv, ASSIGNMENT_TYPE_OVVERIDE_OPTION);
    
    if (assignmentTypeString) {
        // the user is asking to only be assigned to a particular type of assignment
        // so set that as the ::overridenAssignmentType to be used in requests
        ::overiddenAssignmentType = (Assignment::Type) atoi(assignmentTypeString);
    }
    
    const char ASSIGNMENT_POOL_OPTION[] = "--pool";
    ::assignmentPool = getCmdOption(argc, (const char**) argv, ASSIGNMENT_POOL_OPTION);

    const char* NUM_FORKS_PARAMETER = "-n";
    const char* numForksString = getCmdOption(argc, (const char**)argv, NUM_FORKS_PARAMETER);
    
    int processID = 0;
    
    if (numForksString) {
        ::numForks = atoi(numForksString);
        qDebug("Starting %d assignment clients\n", ::numForks);
        
        ::childForks = new pid_t[::numForks];
        
        // fire off as many children as we need (this is one less than the parent since the parent will run as well)
        for (int i = 0; i < ::numForks; i++) {
            processID = fork();
            
            if (processID == 0) {
                // this is in one of the children, break so we don't start a fork bomb
                break;
            } else {
                // this is in the parent, save the ID of the forked process
                childForks[i] = processID;
            }
        }
    }
    
    if (processID == 0 || ::numForks == 0) {
        return childClient();
    } else {
        parentMonitor();
    }
}