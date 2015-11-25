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

function printDebug(message) {
	print(message);
}

(function() {
	var COMMAND_CHANNEL = "com.highfidelity.PlaybackChannel1";
	var ANNOUNCE_CHANNEL = "com.highfidelity.playbackAgent.announceID";

	// The time between alive messages on the command channel
	var ALIVE_PERIOD = 3;
	var NUM_CYCLES_BEFORE_RESET = 5;

	// Service Actions
	var AGENT_READY = "ready";
	var INVALID_ACTOR = -2;

	var MASTER_INDEX = -1;

	var MASTER_ALIVE = -1;
	var MASTER_FIRE_AGENT = -2;

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
	//---------------------------------
	var MasterController = function() {
    	this.timeSinceLastAlive = 0;
		this.knownAgents = new Array;
    	this.subscribed = false;
    };

	MasterController.prototype.destroy = function() {
	    if (this.subscribed) {    	
	    	Messages.unsubscribe(ANNOUNCE_CHANNEL);
		    Messages.unsubscribe(COMMAND_CHANNEL);
		    this.subscribed = true;
		}
	};

	MasterController.prototype.reset = function() {
		this.timeSinceLastAlive = 0;

		if (!this.subscribed) {
		    Messages.subscribe(COMMAND_CHANNEL);
			Messages.subscribe(ANNOUNCE_CHANNEL);
			var localThis = this;
			Messages.messageReceived.connect(function (channel, message, senderID) {
				if (channel == ANNOUNCE_CHANNEL) {
					localThis._processAnnounceMessage(message, senderID);
				    return;
				}  
			});
		}
		// ready to roll, enable
	    this.subscribed = true;
		printDebug("Master Started");
    };

    MasterController.prototype._processAnnounceMessage = function(message, senderID) {
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
	//---------------------------------
	var AgentController = function() {
    	this.subscribed = false; 
    	
    	this.timeSinceLastAlive = 0;
    	this.numCyclesWithoutAlive = 0;
    	this.actorIndex = INVALID_ACTOR;
    	this.notifyAlive = false;

    	this.onHired = function() {};
    	this.onCommand = function(command) {};
    	this.onFired = function() {};
    };

	AgentController.prototype.destroy = function() {
	    if (this.subscribed) {  
	    	this.fire();  	
	    	Messages.unsubscribe(ANNOUNCE_CHANNEL);
		    Messages.unsubscribe(COMMAND_CHANNEL);
		    this.subscribed = true;
		}
	};

	AgentController.prototype.reset = function() {
		// If already hired, fire
		this.fired();

		if (!this.subscribed) {
		    Messages.subscribe(COMMAND_CHANNEL);
			Messages.subscribe(ANNOUNCE_CHANNEL);
			var localThis = this;
			Messages.messageReceived.connect(function (channel, message, senderID) {
				if (channel == ANNOUNCE_CHANNEL) {
					localThis._processAnnounceMessage(message, senderID);
				    return;
				}
				if (channel == COMMAND_CHANNEL) {
					localThis._processCommandMessage(message, senderID);
					return;
				}   
			});
		}
	    this.subscribed = true;
		printDebug("Client Started");
    };

    AgentController.prototype._processAnnounceMessage = function(message, senderID) {
		//printDebug("Client " + this.actorIndex + " Received Announcement = " + message);
		if (message != AGENT_READY) {
			// this may be a message to hire me if i m not already
			if (this.actorIndex == INVALID_ACTOR) {
	          	var parts = message.split(".");
	            var agentID = parts[0];
	            var actorIndex = parts[1];
	            //printDebug("Client " + Agent.sessionUUID + " - " + agentID + " Hired!");
	            if (agentID == Agent.sessionUUID) {
	                this.actorIndex = actorIndex;
	            	printDebug("Client " + this.actorIndex + " Hired!");
	            	this.onHired();		    
	            //    Messages.unsubscribe(ANNOUNCE_CHANNEL); // id announce channel
	            }
	        }
	    }
    }

    AgentController.prototype._processCommandMessage = function(message, senderID) {			
		var command = unpackMessage(message);
		//printDebug("Received command = " + JSON.stringify(command));

		if (command.id_key == localThis.actorIndex || command.id_key == MASTER_INDEX) {
			if (command.action_key == MASTER_ALIVE) {
				this.notifyAlive = true;
			} else if (command.action_key == MASTER_FIRE_AGENT) {
			    printDebug("Master firing Agent");
				this.fire();
			} else {
				printDebug("True action received = " + JSON.stringify(command) + senderID);	
				this.onCommand(command);
			}
		} else {
			// ignored
		}
    };

	AgentController.prototype.update = function(deltaTime) {
	    this.timeSinceLastAlive += deltaTime;
	    if (this.timeSinceLastAlive > ALIVE_PERIOD) {
			if (this.subscribed) {
	        	if (this.actorIndex == INVALID_ACTOR) {
            		Messages.sendMessage(ANNOUNCE_CHANNEL, AGENT_READY);
            		//printDebug("Client Ready" + ANNOUNCE_CHANNEL + AGENT_READY); 
            	} else {
            		this.numCyclesWithoutAlive++;
					if (this.notifyAlive) {
			            this.notifyAlive = false;
			            this.numCyclesWithoutAlive = 0;
			            printDebug("Master Alive");            
			        } else if (this.numCyclesWithoutAlive > NUM_CYCLES_BEFORE_RESET) {
			            printDebug("Master Lost, firing Agent");
			            this.fired();           
			        }
            	}
        	}

	        this.timeSinceLastAlive = 0;	
	    }
	};

	AgentController.prototype.fired = function() {
		// clear the state first
		var wasHired = (this.actorIndex != INVALID_ACTOR);
		this.actorIndex= INVALID_ACTOR;	
        this.notifyAlive = false;
        this.timeSinceLastAlive = 0;
        this.numCyclesWithoutAlive = 0;
        // then custom fire if was hired
		if (wasHired) {
			this.onFired();
		}		   	
	}


	this.AgentController = AgentController;
})();



