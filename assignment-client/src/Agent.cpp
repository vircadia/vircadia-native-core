//
//  Agent.cpp
//  hifi
//
//  Created by Stephen Birarda on 7/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <curl/curl.h>

#include <QtScript/QScriptEngine>

#include <AvatarData.h>
#include <NodeList.h>
#include <UUID.h>
#include <VoxelConstants.h>

#include "Agent.h"
#include "voxels/VoxelScriptingInterface.h"

Agent::Agent(const unsigned char* dataBuffer, int numBytes) :
    Assignment(dataBuffer, numBytes),
    _shouldStop(false)
{
    
}

void Agent::stop() {
    _shouldStop = true;
}

static size_t writeScriptDataToString(void *contents, size_t size, size_t nmemb, void *userdata) {
    size_t realSize = size * nmemb;
    
    QString* scriptContents = (QString*) userdata;
    
    // append this chunk to the scriptContents
    scriptContents->append(QByteArray((char*) contents, realSize));
    
    // return the amount of data read
    return realSize;
}

void Agent::run() {
    NodeList* nodeList = NodeList::getInstance();
    nodeList->setOwnerType(NODE_TYPE_AGENT);
    nodeList->setNodeTypesOfInterest(&NODE_TYPE_VOXEL_SERVER, 1);

    nodeList->getNodeSocket()->setBlocking(false);
    
    // figure out the URL for the script for this agent assignment
    QString scriptURLString("http://%1:8080/assignment/%2");
    scriptURLString = scriptURLString.arg(NodeList::getInstance()->getDomainIP().toString(),
                                          uuidStringWithoutCurlyBraces(_uuid));
    
    // setup curl for script download
    CURLcode curlResult;
    
    CURL* curlHandle = curl_easy_init();
    
    // tell curl which file to grab
    curl_easy_setopt(curlHandle, CURLOPT_URL, scriptURLString.toStdString().c_str());
    
    // send the data to the WriteMemoryCallback function
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, writeScriptDataToString);
    
    QString scriptContents;
    
    // pass the scriptContents QString to append data to
    curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, (void *)&scriptContents);
    
    // send a user agent since some servers will require it
    curl_easy_setopt(curlHandle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    
    qDebug() << "Downloading script at" << scriptURLString << "\n";
    
    // blocking get for JS file
    curlResult = curl_easy_perform(curlHandle);
  
    if (curlResult == CURLE_OK) {
        // cleanup curl
        curl_easy_cleanup(curlHandle);
        curl_global_cleanup();
        
        QScriptEngine engine;
        
        QScriptValue agentValue = engine.newQObject(this);
        engine.globalObject().setProperty("Agent", agentValue);
        
        VoxelScriptingInterface voxelScripter;
        QScriptValue voxelScripterValue =  engine.newQObject(&voxelScripter);
        engine.globalObject().setProperty("Voxels", voxelScripterValue);
        
        QScriptValue treeScaleValue = engine.newVariant(QVariant(TREE_SCALE));
        engine.globalObject().setProperty("TREE_SCALE", treeScaleValue);
        
        const long long VISUAL_DATA_SEND_INTERVAL_USECS = (1 / 60.0f) * 1000 * 1000;

        // let the VoxelPacketSender know how frequently we plan to call it
        voxelScripter.getVoxelPacketSender()->setProcessCallIntervalHint(VISUAL_DATA_SEND_INTERVAL_USECS);
        
        QScriptValue visualSendIntervalValue = engine.newVariant((QVariant(VISUAL_DATA_SEND_INTERVAL_USECS / 1000)));
        engine.globalObject().setProperty("VISUAL_DATA_SEND_INTERVAL_MS", visualSendIntervalValue);
        
        qDebug() << "Downloaded script:" << scriptContents << "\n";
        QScriptValue result = engine.evaluate(scriptContents);
        qDebug() << "Evaluated script.\n";
        
        if (engine.hasUncaughtException()) {
            int line = engine.uncaughtExceptionLineNumber();
            qDebug() << "Uncaught exception at line" << line << ":" << result.toString() << "\n";
        }
        
        timeval thisSend;
        timeval lastDomainServerCheckIn = {};
        int numMicrosecondsSleep = 0;
        
        sockaddr_in senderAddress;
        unsigned char receivedData[MAX_PACKET_SIZE];
        ssize_t receivedBytes;
        
        bool hasVoxelServer = false;
        
        while (!_shouldStop) {
            // update the thisSend timeval to the current time
            gettimeofday(&thisSend, NULL);
            
            // if we're not hearing from the domain-server we should stop running
            if (NodeList::getInstance()->getNumNoReplyDomainCheckIns() == MAX_SILENT_DOMAIN_SERVER_CHECK_INS) {
                break;
            }
            
            // send a check in packet to the domain server if DOMAIN_SERVER_CHECK_IN_USECS has elapsed
            if (usecTimestampNow() - usecTimestamp(&lastDomainServerCheckIn) >= DOMAIN_SERVER_CHECK_IN_USECS) {
                gettimeofday(&lastDomainServerCheckIn, NULL);
                NodeList::getInstance()->sendDomainServerCheckIn();
            }
            
            if (!hasVoxelServer) {
                for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
                    if (node->getType() == NODE_TYPE_VOXEL_SERVER) {
                        hasVoxelServer = true;
                    }
                }
            }
            
            if (hasVoxelServer) {
                // allow the scripter's call back to setup visual data
                emit willSendVisualDataCallback();
                
                if (engine.hasUncaughtException()) {
                    int line = engine.uncaughtExceptionLineNumber();
                    qDebug() << "Uncaught exception at line" << line << ":" << engine.uncaughtException().toString() << "\n";
                }
                
                // release the queue of edit voxel messages.
                voxelScripter.getVoxelPacketSender()->releaseQueuedMessages();

                // since we're in non-threaded mode, call process so that the packets are sent
                voxelScripter.getVoxelPacketSender()->process();
            }
            
            while (NodeList::getInstance()->getNodeSocket()->receive((sockaddr*) &senderAddress, receivedData, &receivedBytes)) {
                NodeList::getInstance()->processNodeData((sockaddr*) &senderAddress, receivedData, receivedBytes);
            }
            
            // sleep for the correct amount of time to have data send be consistently timed
            if ((numMicrosecondsSleep = VISUAL_DATA_SEND_INTERVAL_USECS - (usecTimestampNow() - usecTimestamp(&thisSend))) > 0) {
                usleep(numMicrosecondsSleep);
            }
        }
        
    } else {
        // error in curl_easy_perform
        qDebug() << "curl_easy_perform for JS failed:" << curl_easy_strerror(curlResult) << "\n";
        
        // cleanup curl
        curl_easy_cleanup(curlHandle);
        curl_global_cleanup();
    }
}
