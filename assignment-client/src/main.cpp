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

#include <Assignment.h>
#include <AudioMixer.h>
#include <AvatarMixer.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>

const long long ASSIGNMENT_REQUEST_INTERVAL_USECS = 1 * 1000 * 1000;

pid_t* childForks = NULL;
sockaddr_in customAssignmentSocket = {};
const char* assignmentPool = NULL;
int numForks = 0;

void childClient() {
    // this is one of the child forks or there is a single assignment client, continue assignment-client execution
    
    // create a NodeList as an unassigned client
    NodeList* nodeList = NodeList::createInstance(NODE_TYPE_UNASSIGNED);
    
    // set the custom assignment socket if we have it
    if (customAssignmentSocket.sin_addr.s_addr != 0) {
        nodeList->setAssignmentServerSocket((sockaddr*) &customAssignmentSocket);
    }
    
    // change the timeout on the nodelist socket to be as often as we want to re-request
    nodeList->getNodeSocket()->setBlockingReceiveTimeoutInUsecs(ASSIGNMENT_REQUEST_INTERVAL_USECS);
    
    timeval lastRequest = {};
    
    unsigned char packetData[MAX_PACKET_SIZE];
    ssize_t receivedBytes = 0;
    
    // create a request assignment, accept all assignments, pass the desired pool (if it exists)
    Assignment requestAssignment(Assignment::Request, Assignment::All, assignmentPool);
    
    while (true) {
        if (usecTimestampNow() - usecTimestamp(&lastRequest) >= ASSIGNMENT_REQUEST_INTERVAL_USECS) {
            gettimeofday(&lastRequest, NULL);
            // if we're here we have no assignment, so send a request
            nodeList->sendAssignment(requestAssignment);
        }
        
        if (nodeList->getNodeSocket()->receive(packetData, &receivedBytes) &&
            packetData[0] == PACKET_TYPE_DEPLOY_ASSIGNMENT && packetVersionMatch(packetData)) {
            
            // construct the deployed assignment from the packet data
            Assignment deployedAssignment(packetData, receivedBytes);
            
            qDebug() << "Received an assignment - " << deployedAssignment << "\n";
            
            // switch our nodelist DOMAIN_IP to the ip receieved in the assignment
            if (deployedAssignment.getDomainSocket()->sa_family == AF_INET) {
                in_addr domainSocketAddr = ((sockaddr_in*) deployedAssignment.getDomainSocket())->sin_addr;
                nodeList->setDomainIP(inet_ntoa(domainSocketAddr));
                
                qDebug() << "Changed domain IP to " << inet_ntoa(domainSocketAddr);
            }
            
            if (deployedAssignment.getType() == Assignment::AudioMixer) {
                AudioMixer::run();
            } else {
                AvatarMixer::run();
            }
            
            qDebug() << "Assignment finished or never started - waiting for new assignment";
            
            // reset our NodeList by switching back to unassigned and clearing the list
            nodeList->setOwnerType(NODE_TYPE_UNASSIGNED);
            nodeList->clear();
        }
    }
}

void sigchldHandler(int sig) {
    pid_t processID;
    int status;
    
    while ((processID = waitpid(-1, &status, WNOHANG)) != -1) {
        if (processID == 0) {
            // there are no more children to process, break out of here
            break;
        }
        
        qDebug() << "Handling death of" << processID;
        
        int newForkProcessID = 0;
        
        // find the dead process in the array of child forks
        for (int i = 0; i < ::numForks; i++) {
            if (::childForks[i] == processID) {
                qDebug() << "Matched" << ::childForks[i] << "with" << processID;
                
                newForkProcessID = fork();
                if (newForkProcessID == 0) {
                    // this is the child, call childClient
                    childClient();
                    
                    // break out so we don't fork bomb
                    break;
                } else {
                    // this is the parent, replace the dead process with the new one
                    ::childForks[i] = newForkProcessID;
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

int main(int argc, const char* argv[]) {
    
    setvbuf(stdout, NULL, _IOLBF, 0);
    
    // grab the overriden assignment-server hostname from argv, if it exists
    const char* customAssignmentServer = getCmdOption(argc, argv, "-a");
    if (customAssignmentServer) {
        ::customAssignmentSocket = socketForHostnameAndHostOrderPort(customAssignmentServer, ASSIGNMENT_SERVER_PORT);
    }
    
    const char* NUM_FORKS_PARAMETER = "-n";
    const char* numForksString = getCmdOption(argc, argv, NUM_FORKS_PARAMETER);
    
    // grab the assignment pool from argv, if it was passed
    const char* ASSIGNMENT_POOL_PARAMETER = "-p";
    ::assignmentPool = getCmdOption(argc, argv, ASSIGNMENT_POOL_PARAMETER);
    
    int processID = 0;
    
    if (numForksString) {
        ::numForks = atoi(numForksString);
        qDebug() << "Starting" << numForks << "assignment clients.";
        
        ::childForks = new pid_t[numForks];
        
        // fire off as many children as we need (this is one less than the parent since the parent will run as well)
        for (int i = 0; i < numForks; i++) {
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
    
    if (processID == 0 || numForks == 0) {
        childClient();
    } else {
        parentMonitor();
    }
}