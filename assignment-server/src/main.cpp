//
//  main.cpp
//  assignment-server
//
//  Created by Stephen Birarda on 7/1/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <arpa/inet.h>
#include <fstream>
#include <queue>

#include <QtCore/QString>

#include <Assignment.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UDPSocket.h>

const int MAX_PACKET_SIZE_BYTES = 1400;
const long long NUM_DEFAULT_ASSIGNMENT_STALENESS_USECS = 10 * 1000 * 1000;

int main(int argc, const char* argv[]) {
    
    std::deque<Assignment*> assignmentQueue;
    
    sockaddr_in senderSocket;
    unsigned char senderData[MAX_PACKET_SIZE_BYTES] = {};
    ssize_t receivedBytes = 0;
    
    UDPSocket serverSocket(ASSIGNMENT_SERVER_PORT);
    
    unsigned char assignmentPacket[MAX_PACKET_SIZE_BYTES];
    int numSendHeaderBytes = populateTypeAndVersion(assignmentPacket, PACKET_TYPE_DEPLOY_ASSIGNMENT);
    
    while (true) {
        if (serverSocket.receive((sockaddr*) &senderSocket, &senderData, &receivedBytes)) {
            if (senderData[0] == PACKET_TYPE_REQUEST_ASSIGNMENT) {
                // construct the requested assignment from the packet data
                Assignment requestAssignment(senderData, receivedBytes);
                
                qDebug() << "Received request for assignment:" << requestAssignment;
                qDebug() << "Current queue size is" << assignmentQueue.size();
                
                // make sure there are assignments in the queue at all
                if (assignmentQueue.size() > 0) {
                    
                    std::deque<Assignment*>::iterator assignment = assignmentQueue.begin();
                    
                    // enumerate assignments until we find one to give this client (if possible)
                    while (assignment != assignmentQueue.end()) {
                        
                        // if this assignment is stale then get rid of it and check the next one
                        if (usecTimestampNow() - usecTimestamp(&((*assignment)->getTime()))
                            >= NUM_DEFAULT_ASSIGNMENT_STALENESS_USECS) {
                            delete *assignment;
                            assignment = assignmentQueue.erase(assignment);
                            
                            continue;
                        }
                        
                        // check if the requestor is on the same network as the destination for the assignment
                        if (senderSocket.sin_addr.s_addr ==
                            ((sockaddr_in*) (*assignment)->getAttachedPublicSocket())->sin_addr.s_addr) {
                            // if this is the case we remove the public socket on the assignment by setting it to NULL
                            // this ensures the local IP and port sent to the requestor is the local address of destination
                            (*assignment)->setAttachedPublicSocket(NULL);
                        }
                        
                        
                        int numAssignmentBytes = (*assignment)->packToBuffer(assignmentPacket + numSendHeaderBytes);
                        
                        // send the assignment
                        serverSocket.send((sockaddr*) &senderSocket,
                                          assignmentPacket,
                                          numSendHeaderBytes + numAssignmentBytes);
                        
                        
                        // delete this assignment now that it has been sent out
                        delete *assignment;
                        // remove it from the deque and make the iterator the next assignment
                        assignmentQueue.erase(assignment);
                        
                        // stop looping - we've handed out an assignment
                        break;
                    }
                }
            } else if (senderData[0] == PACKET_TYPE_CREATE_ASSIGNMENT && packetVersionMatch(senderData)) {
                // construct the create assignment from the packet data
                Assignment* createdAssignment = new Assignment(senderData, receivedBytes);
                
                qDebug() << "Received a created assignment:" << *createdAssignment;
                qDebug() << "Current queue size is" << assignmentQueue.size();
                
                // assignment server is likely on a public server
                // assume that the address we now have for the sender is the public address/port
                // and store that with the assignment so it can be given to the requestor later if necessary
                createdAssignment->setAttachedPublicSocket((sockaddr*) &senderSocket);
                
                // add this assignment to the queue
                assignmentQueue.push_back(createdAssignment);
            }
        }
    }    
}
