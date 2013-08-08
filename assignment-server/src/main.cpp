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

#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UDPSocket.h>

const int MAX_PACKET_SIZE_BYTES = 1400;

struct Assignment {
    QString scriptFilename;
};

int main(int argc, const char* argv[]) {
    
    std::queue<Assignment> assignmentQueue;
    
    sockaddr_in senderSocket;
    unsigned char senderData[MAX_PACKET_SIZE_BYTES] = {};
    ssize_t receivedBytes = 0;
    
    UDPSocket serverSocket(ASSIGNMENT_SERVER_PORT);
    
    int numHeaderBytes = numBytesForPacketHeader((unsigned char*) &PACKET_TYPE_SEND_ASSIGNMENT);
    unsigned char assignmentPacket[numHeaderBytes + sizeof(char)];
    populateTypeAndVersion(assignmentPacket, PACKET_TYPE_SEND_ASSIGNMENT);
    
    while (true) {
        if (serverSocket.receive((sockaddr*) &senderSocket, &senderData, &receivedBytes)) {
            
            int numHeaderBytes = numBytesForPacketHeader(senderData);
            
            if (senderData[0] == PACKET_TYPE_REQUEST_ASSIGNMENT) {       
                qDebug() << "Assignment request received.\n";
                // grab the FI assignment in the queue, if it exists
                if (assignmentQueue.size() > 0) {
                    Assignment firstAssignment = assignmentQueue.front();
                    assignmentQueue.pop();
                    
                    QString scriptURL = QString("http://base8-compute.s3.amazonaws.com/%1").arg(firstAssignment.scriptFilename);
                    
                    qDebug() << "Sending assignment with URL" << scriptURL << "\n";
                    
                    int scriptURLBytes = scriptURL.size();
                    memcpy(assignmentPacket + numHeaderBytes, scriptURL.toLocal8Bit().constData(), scriptURLBytes);

                    // send the assignment
                    serverSocket.send((sockaddr*) &senderSocket, assignmentPacket, numHeaderBytes + scriptURLBytes);
                }
            } else if (senderData[0] == PACKET_TYPE_SEND_ASSIGNMENT) {
                Assignment newAssignment;
                
                senderData[receivedBytes] = '\0';
                newAssignment.scriptFilename = QString((const char*)senderData + numHeaderBytes);
                
                qDebug() << "Added an assignment with script with filename" << newAssignment.scriptFilename << "\n";
                                
                // add this assignment to the queue
                
                // we're not a queue right now, only keep one assignment
                if (assignmentQueue.size() > 0) {
                    assignmentQueue.pop();
                }
                
                assignmentQueue.push(newAssignment);
            }
        }
    }    
}
