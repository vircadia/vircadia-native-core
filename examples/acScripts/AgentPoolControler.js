//
//  AgentPoolController.js
//  acScripts
//
//  Created by Sam Gateau on 11/23/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var COMMAND_CHANNEL = "com.highfidelity.PlaybackChannel1";
var ANNOUNCE_CHANNEL = "com.highfidelity.playbackAgent.announceID";

// The time between alive messages on the command channel
var ALIVE_PERIOD = 1;

var NUM_CYCLES_BEFORE_RESET = 5;

// Service Actions
var AGENT_READY = "ready";
var INVALID_ACTOR = -2;

var MASTER_INDEX = -1;

var MASTER_ALIVE = -1;

function printDebug(message) {
	print(message);
}

(function() {

	var makeMessage = function(id, action, argument) {
		var message = {
            id_key: id,
            action_key: action,
            argument_key: argument
	    };
	    return message;
	};

	var unpackMessage = function(message) {
		return JSON.parse(message);
	};
	
	// master side
	var MasterController = function() {
    	this.timeSinceLastAlive = 0;
		this.knownAgents = new Array;
    	this.subscribed = false;
    };

	MasterController.prototype.reset = function() {
		if (this.subscribed) {
			this.timeSinceLastAlive = 0;

		}

	    Messages.subscribe(COMMAND_CHANNEL);
		Messages.subscribe(ANNOUNCE_CHANNEL);

	    this.timeSinceLastAlive = 0;

	    var localThis = this;

		Messages.messageReceived.connect(function (channel, message, senderID) {
			printDebug("Agent received");
			if (channel == ANNOUNCE_CHANNEL) {
				printDebug("Agent received");
			//	localThis._processAgentMessage(message, senderID);
			}
		});

		// ready to roll, enable
	    this.subscribed = true;
    };

    MasterController.prototype._processAgentMessage = function(message, senderID) {
		if (message == AGENT_READY) {

			// check to see if we know about this agent
			if (this.knownAgents.indexOf(senderID) < 0) {

				var indexOfNewAgent = this.knownAgents.length;
				this.knownAgents[indexOfNewAgent] = senderID;
				printDebug("New agent available to be hired " + senderID);
				
				var acknowledgeMessage = senderID + "." + indexOfNewAgent;
				Messages.sendMessage(ANNOUNCE_CHANNEL, acknowledgeMessage);
			} else {

				printDebug("New agent still sending ready ? " + senderID);
			}
		}
    };

	MasterController.prototype.destroy = function() {
	    if (this.subscribed) {
	    	Messages.unsubscribe(ANNOUNCE_CHANNEL);
		    Messages.unsubscribe(COMMAND_CHANNEL);
		    this.timeSinceLastAlive = 0;
		    this.subscribed = true;
		}
	};
   
    MasterController.prototype.sendCommand = function(target, action, argument) {      
        if (this.subscribed) {
        	var messageJSON = JSON.stringify(makeMessage(target, action, argument));
            Messages.sendMessage(COMMAND_CHANNEL, messageJSON);
            printDebug("Master sent message: " + messageJSON);
        }
	};

	MasterController.prototype.update = function(deltaTime) {
	    this.timeSinceLastAlive += deltaTime;
	    if (this.timeSinceLastAlive > ALIVE_PERIOD) {
	        this.timeSinceLastAlive = 0;
	        printDebug("Master ping alive");
	        this.sendCommand(MASTER_INDEX, MASTER_ALIVE);
	    }
	};


	this.MasterController = MasterController;

	// agent side
	var AgentController = function() {
    	this.timeSinceLastAlive = 0;
    	this.subscribed = false;
    	this.actorIndex = INVALID_ACTOR;
    	this.notifyAlive = false;
    };

	AgentController.prototype.reset = function() {
		if (!this.subscribed) {
		    Messages.subscribe(COMMAND_CHANNEL);
			Messages.subscribe(ANNOUNCE_CHANNEL);
		}

	    this.timeSinceLastAlive = 0;

	    var localThis = this;
		Messages.messageReceived.connect(function (channel, message, senderID) {
			if (channel == ANNOUNCE_CHANNEL) {
				if (message != AGENT_READY) {
					// this may be a message to hire me if i m not already
					if (id == INVALID_ACTOR) {
			            var parts = message.split(".");
			            var agentID = parts[0];
			            var actorIndex = parts[1];
			            if (agentID == Agent.sessionUUID) {
			           //     localThis.actorIndex = agentIndex;
			                Messages.unsubscribe(ANNOUNCE_CHANNEL); // id announce channel
			            }
			        }
			    }
			    return;
			}
			if (channel == COMMAND_CHANNEL) {
				var command = unpackMessage(message);
				printDebug("Received command = " == command);

				if (command.id_key == localThis.actorIndex || command.id_key == MASTER_INDEX) {
					if (command.action_key == MASTER_ALIVE) {
				//		localThis.notifyAlive = true;
					} else {
				//		localThis._processMasterMessage(command, senderID);	
					}
				} else {
					// ignored
				}
				return;
			}   
		});

		// ready to roll, enable
	    this.subscribed = true;
				printDebug("In Reset");
    };

    AgentController.prototype._processMasterMessage = function(command, senderID) {
		printDebug("True action received = " + JSON.stringify(command) + senderID);	
    };

	AgentController.prototype.update = function(deltaTime) {
	    this.timeSinceLastAlive += deltaTime;
	    if (this.timeSinceLastAlive > ALIVE_PERIOD) { 
			if (this.subscribed) {
				printDebug("In update of client");
	        	if (this.actorIndex == INVALID_ACTOR) {
            		Messages.sendMessage(ANNOUNCE_CHANNEL, AGENT_READY);
            		printDebug("Client Ready"); 
            	} else {
            		if (this.notifyAlive) {
			            this.notifyAlive = false;
			            printDebug("Master Alive");            
			        } else if (this.timeSinceLastAlive > NUM_CYCLES_BEFORE_RESET * ALIVE_PERIOD) {
			            printDebug("Master Lost, reseting Agent");
			            this.actorIndex = UNKNOWN_AGENT_ID;           
			        }
            	}
        	}

	        this.timeSinceLastAlive = 0;	
	    }
	};


	this.AgentController = AgentController;
})();



