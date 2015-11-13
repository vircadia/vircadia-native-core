//
//  synchronizerEntityScript.js
//  examples/entityScripts
//
//  Created by Alessandro Signa on 11/12/15.
//  Copyright 2015 High Fidelity, Inc.
//

//  This script shows how to create a synchronized event between avatars trhough an entity.
//  It works using the entity's userData: the master change its value and every client checks it every frame
//  This entity prints a message when the event starts and when it ends. 
//  The client running synchronizerMaster.js is the event master and it decides when the event starts/ends by pressing a button.
//  All the avatars in the area when the master presses the button will receive a message.
//  

//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html




(function() {
    var insideArea = false;
    var isJoiningTheEvent = false;
    var _this;
    
    function update(){
        var userData = JSON.parse(Entities.getEntityProperties(_this.entityID, ["userData"]).userData);
        var valueToCheck = userData.myKey.valueToCheck;
        if(valueToCheck && !isJoiningTheEvent){
            _this.sendMessage();
        }else if((!valueToCheck && isJoiningTheEvent) || (isJoiningTheEvent && !insideArea)){
            _this.stopMessage();
        }
        
    }

    function ParamsEntity() {
        _this = this;
        return;
    }
    
    
    ParamsEntity.prototype = {
        preload: function(entityID) {
            print('entity loaded')
            this.entityID = entityID;
            Script.update.connect(update);
        },
        enterEntity: function(entityID) {       
            print("enterEntity("+entityID+")");
            var userData = JSON.parse(Entities.getEntityProperties(_this.entityID, ["userData"]).userData);
            var valueToCheck = userData.myKey.valueToCheck;
            if(!valueToCheck){
                //i'm in the area in time (before the event starts)
                insideArea = true;
            }
            change(entityID);
        },
        leaveEntity: function(entityID) {      
            print("leaveEntity("+entityID+")");
            Entities.editEntity(entityID, { color: { red: 255, green: 190, blue: 20} });
            insideArea = false;
        },
        
        sendMessage: function(myID){
            if(insideArea && !isJoiningTheEvent){
                print("The event started");
                isJoiningTheEvent = true;
            }
        },
        
        stopMessage: function(myID){
            if(isJoiningTheEvent){
                print("The event ended");
                isJoiningTheEvent = false;
            }
        }
    }
    
    function change(entityID) {
        Entities.editEntity(entityID, { color: { red: 255, green: 100, blue: 220} });
    }
    

    return new ParamsEntity();
});
