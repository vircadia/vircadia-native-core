//
//  recordingEntityScript.js
//  examples/entityScripts
//
//  Created by Alessandro Signa on 11/12/15.
//  Copyright 2015 High Fidelity, Inc.
//

//  All the avatars in the area when the master presses the button will start/stop recording.
//  

//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html




(function() {
    var insideRecorderArea = false;
    var enteredInTime = false;
    var isAvatarRecording = false;
    var _this;

    function recordingEntity() {
        _this = this;
        return;
    }
    
    recordingEntity.prototype = {
        update: function(){
            var userData = JSON.parse(Entities.getEntityProperties(_this.entityID, ["userData"]).userData);
            var isRecordingStarted = userData.recordingKey.isRecordingStarted;
            if(isRecordingStarted && !isAvatarRecording){
                _this.startRecording();
            }else if((!isRecordingStarted && isAvatarRecording) || (isAvatarRecording && !insideRecorderArea)){
                _this.stopRecording();
            }else if(!isRecordingStarted && insideRecorderArea && !enteredInTime){
                //if an avatar enters the zone while a recording is started he will be able to participate to the next group recording
                enteredInTime = true;
            }
            
        },
        preload: function(entityID) {
            this.entityID = entityID;
            Script.update.connect(_this.update);
        },
        enterEntity: function(entityID) {       
            print("entering in the recording area");
            insideRecorderArea = true;
            var userData = JSON.parse(Entities.getEntityProperties(_this.entityID, ["userData"]).userData);
            var isRecordingStarted = userData.recordingKey.isRecordingStarted;
            if(!isRecordingStarted){
                //i'm in the recording area in time (before the event starts)
                enteredInTime = true;
            }
        },
        leaveEntity: function(entityID) {      
            print("leaving the recording area");
            insideRecorderArea = false;
            enteredInTime = false;
        },
        
        startRecording: function(entityID){
            if(enteredInTime && !isAvatarRecording){
                print("RECORDING STARTED");
                Recording.startRecording();
                isAvatarRecording = true;
            }
        },
        
        stopRecording: function(entityID){
            if(isAvatarRecording){
                print("RECORDING ENDED");
                Recording.stopRecording();
                Recording.loadLastRecording();
                isAvatarRecording = false;
                recordingFile = Window.save("Save recording to file", "./groupRecording", "Recordings (*.hfr)");
                if (!(recordingFile === "null" || recordingFile === null || recordingFile === "")) {
                    Recording.saveRecording(recordingFile);
                }
            }
        },
        clean: function(entityID) {
            Script.update.disconnect(_this.update);
        }
    }
    
    

    return new recordingEntity();
});
