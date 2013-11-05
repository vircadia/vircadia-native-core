//
//  Agent.cpp
//  hifi
//
//  Created by Stephen Birarda on 7/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <curl/curl.h>

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

QScriptValue vec3toScriptValue(QScriptEngine *engine, const glm::vec3 &vec3) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", vec3.x);
    obj.setProperty("y", vec3.y);
    obj.setProperty("z", vec3.z);
    return obj;
}

void vec3FromScriptValue(const QScriptValue &object, glm::vec3 &vec3) {
    vec3.x = object.property("x").toVariant().toFloat();
    vec3.y = object.property("y").toVariant().toFloat();
    vec3.z = object.property("z").toVariant().toFloat();
}

void Agent::run() {
    NodeList* nodeList = NodeList::getInstance();
    nodeList->setOwnerType(NODE_TYPE_AGENT);
    
    const char AGENT_NODE_TYPES_OF_INTEREST[2] = { NODE_TYPE_VOXEL_SERVER, NODE_TYPE_AUDIO_MIXER };
    
    nodeList->setNodeTypesOfInterest(AGENT_NODE_TYPES_OF_INTEREST, sizeof(AGENT_NODE_TYPES_OF_INTEREST));

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
    
    // make sure CURL fails on a 400 code
    curl_easy_setopt(curlHandle, CURLOPT_FAILONERROR, true);
    
    qDebug() << "Downloading script at" << scriptURLString << "\n";
    
    // blocking get for JS file
    curlResult = curl_easy_perform(curlHandle);
  
    if (curlResult == CURLE_OK) {
        // cleanup curl
        curl_easy_cleanup(curlHandle);
        curl_global_cleanup();
        
        QScriptEngine engine;
        
        // register meta-type for glm::vec3 conversions
        qScriptRegisterMetaType(&engine, vec3toScriptValue, vec3FromScriptValue);
        
        QScriptValue agentValue = engine.newQObject(this);
        engine.globalObject().setProperty("Agent", agentValue);
        
        VoxelScriptingInterface voxelScripter;
        QScriptValue voxelScripterValue =  engine.newQObject(&voxelScripter);
        engine.globalObject().setProperty("Voxels", voxelScripterValue);
        
        QScriptValue treeScaleValue = engine.newVariant(QVariant(TREE_SCALE));
        engine.globalObject().setProperty("TREE_SCALE", treeScaleValue);

        // let the VoxelPacketSender know how frequently we plan to call it
        voxelScripter.getVoxelPacketSender()->setProcessCallIntervalHint(INJECT_INTERVAL_USECS);
        
        // hook in a constructor for audio injectorss
        AudioInjector scriptedAudioInjector(BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
        QScriptValue audioInjectorValue = engine.newQObject(&scriptedAudioInjector);
        engine.globalObject().setProperty("AudioInjector", audioInjectorValue);
        
        qDebug() << "Downloaded script:" << scriptContents << "\n";
        QScriptValue result = engine.evaluate(scriptContents);
        qDebug() << "Evaluated script.\n";
        
        if (engine.hasUncaughtException()) {
            int line = engine.uncaughtExceptionLineNumber();
            qDebug() << "Uncaught exception at line" << line << ":" << result.toString() << "\n";
        }
        
        timeval startTime;
        gettimeofday(&startTime, NULL);
        
        timeval lastDomainServerCheckIn = {};
        
        sockaddr_in senderAddress;
        unsigned char receivedData[MAX_PACKET_SIZE];
        ssize_t receivedBytes;
        
        int thisFrame = 0;
        
        NodeList::getInstance()->startSilentNodeRemovalThread();
        
        while (!_shouldStop) {
            
            // if we're not hearing from the domain-server we should stop running
            if (NodeList::getInstance()->getNumNoReplyDomainCheckIns() == MAX_SILENT_DOMAIN_SERVER_CHECK_INS) {
                break;
            }
            
            // send a check in packet to the domain server if DOMAIN_SERVER_CHECK_IN_USECS has elapsed
            if (usecTimestampNow() - usecTimestamp(&lastDomainServerCheckIn) >= DOMAIN_SERVER_CHECK_IN_USECS) {
                gettimeofday(&lastDomainServerCheckIn, NULL);
                NodeList::getInstance()->sendDomainServerCheckIn();
            }
            
            // find the audio-mixer in the NodeList so we can inject audio at it
            Node* audioMixer = NodeList::getInstance()->soloNodeOfType(NODE_TYPE_AUDIO_MIXER);
            
        
            if (audioMixer && audioMixer->getActiveSocket()) {
                emit willSendAudioDataCallback();
            }
            
            int usecToSleep = usecTimestamp(&startTime) + (thisFrame++ * INJECT_INTERVAL_USECS) - usecTimestampNow();
            if (usecToSleep > 0) {
                usleep(usecToSleep);
            }
            
            if (audioMixer && NodeList::getInstance()->getNodeActiveSocketOrPing(audioMixer) && scriptedAudioInjector.hasSamplesToInject()) {
                // we have an audio mixer and samples to inject, send those off
                scriptedAudioInjector.injectAudio(NodeList::getInstance()->getNodeSocket(), audioMixer->getActiveSocket());
                
                // clear out the audio injector so that it doesn't re-send what we just sent
                scriptedAudioInjector.clear();
            }
            
            if (voxelScripter.getVoxelPacketSender()->voxelServersExist()) {
                // allow the scripter's call back to setup visual data
                emit willSendVisualDataCallback();
                
                // release the queue of edit voxel messages.
                voxelScripter.getVoxelPacketSender()->releaseQueuedMessages();
                
                // since we're in non-threaded mode, call process so that the packets are sent
                voxelScripter.getVoxelPacketSender()->process();

            }            
            
            if (engine.hasUncaughtException()) {
                int line = engine.uncaughtExceptionLineNumber();
                qDebug() << "Uncaught exception at line" << line << ":" << engine.uncaughtException().toString() << "\n";
            }

            while (NodeList::getInstance()->getNodeSocket()->receive((sockaddr*) &senderAddress, receivedData, &receivedBytes)
                   && packetVersionMatch(receivedData)) {
                if (receivedData[0] == PACKET_TYPE_VOXEL_JURISDICTION) {
                    voxelScripter.getJurisdictionListener()->queueReceivedPacket((sockaddr&) senderAddress,
                                                                                 receivedData,
                                                                                 receivedBytes);
                } else {
                    NodeList::getInstance()->processNodeData((sockaddr*) &senderAddress, receivedData, receivedBytes);
                }
            }
        }
    
        NodeList::getInstance()->stopSilentNodeRemovalThread(); 
        
    } else {
        // error in curl_easy_perform
        qDebug() << "curl_easy_perform for JS failed:" << curl_easy_strerror(curlResult) << "\n";
        
        // cleanup curl
        curl_easy_cleanup(curlHandle);
        curl_global_cleanup();
    }
}
