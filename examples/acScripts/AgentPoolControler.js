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
    var NUM_CYCLES_BEFORE_RESET = 8;

    // Service Actions
    var MASTER_ID = -1;
    var AGENT_READY = "ready";
    var AGENT_LOST = "agentLost";

    var INVALID_ACTOR = -2;


    var AGENTS_BROADCAST = -1;
    var MASTER_ALIVE = -1;
    var MASTER_FIRE_AGENT = -2;

    var makeUniqueUUID = function(SEUUID) {
        //return SEUUID + Math.random();
        // forget complexity, just give me a four digit pin
        return (Math.random() * 10000).toFixed(0);
    }

    var packAnnounceMessage = function(dest, command, src) {
        var message = {
            dest: dest,
            command: command,
            src: src
        };
        return JSON.stringify(message);
    };

    var unpackAnnounceMessage = function(message) {
        return JSON.parse(message);
    };

    var packCommandMessage = function(dest, action, argument) {
        var message = {
            dest_key: dest,
            action_key: action,
            argument_key: argument
        };
        return JSON.stringify(message);
    };

    var unpackCommandMessage = function(message) {
        return JSON.parse(message);
    };
    
    // Actor
    //---------------------------------
    var Actor = function() {
        this.agentID = INVALID_ACTOR;
        this.onHired = function(actor) {};
        this.onLost = function(actor) {};
    };

    Actor.prototype.isConnected = function () {
        return (this.agentID != INVALID_ACTOR);
    }

    this.Actor = Actor;

    // master side
    //---------------------------------
    var MasterController = function() {
        this.timeSinceLastAlive = 0;
        this.knownAgents = new Array;
        this.hiredActors = new Array;
        this.hiringAgentsQueue = new Array;
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
        var message = unpackAnnounceMessage(message);       
        if (message.dest == MASTER_ID) {
            if (message.command == AGENT_READY) {
                // check to see if we know about this agent
                var agentIndex = this.knownAgents.indexOf(message.src);
                if (agentIndex < 0) {
                    if (this.hiringAgentsQueue.length > 0) {
                        var hiringHandler = this.hiringAgentsQueue.pop();
                        hiringHandler(message.src);
                    } else {
                        //No hiring in queue so bail
                        return;
                    }
                } else {
                    // Master think the agent is hired but not the other way around, forget about it
                    printDebug("New agent still sending ready ? " + message.src + " " + agentIndex + " Forgeting about it");
                    lostActor.agentID = INVALID_ACTOR;
                    lostActor.onLost(lostActor);
                    _clearAgent(agentIndex);
                }
            } else if (message.command == AGENT_LOST) {
                 // check to see if we know about this agent
                var agentIndex = this.knownAgents.indexOf(message.src);
                if (agentIndex >= 0) {
                    lostActor.agentID = INVALID_ACTOR;
                    lostActor.onLost(lostActor);
                    _clearAgent(agentIndex);
                } else {
                    return;
                }
            }
        }
    };
   
    MasterController.prototype.sendCommand = function(target, action, argument) {      
        if (this.subscribed) {
            var messageJSON = packCommandMessage(target, action, argument);
            Messages.sendMessage(COMMAND_CHANNEL, messageJSON);
        }
    };

    MasterController.prototype.update = function(deltaTime) {
        this.timeSinceLastAlive += deltaTime;
        if (this.timeSinceLastAlive > ALIVE_PERIOD) {
            this.timeSinceLastAlive = 0;
            this.sendCommand(AGENTS_BROADCAST, MASTER_ALIVE);
        }
    };


    MasterController.prototype.hireAgent = function(actor) {
        if (actor == null) {
            printDebug("trying to hire an agent with a null actor, abort");
            return;
        }
        var localThis = this;
        this.hiringAgentsQueue.unshift(function(agentID) { 
            printDebug("hiring callback with agent " + agentID+ " " + JSON.stringify(localThis) );
        
            var indexOfNewAgent = localThis.knownAgents.push(agentID)
            actor.agentID = agentID;    
            localThis.hiredActors.push(actor);
                              
            printDebug("New agent available to be hired " + agentID + " " + indexOfNewAgent);                        
            var hireMessage = "HIRE." + indexOfNewAgent;
            var answerMessage = packAnnounceMessage(agentID, hireMessage, MASTER_ID);
            Messages.sendMessage(ANNOUNCE_CHANNEL, answerMessage);
            
            printDebug("message sent calling the actor" + JSON.stringify(actor) );

            actor.onHired(actor); 
        })
    };

    MasterController.prototype.fireAgent = function(actor) {
        // check to see if we know about this agent
        printDebug("MasterController.prototype.fireAgent" + JSON.stringify(actor) + "  " + JSON.stringify(this.knownAgents));
        var actorIndex = this.knownAgents.indexOf(actor.agentID);                
        if (actorIndex >= 0) {
            printDebug("fired actor found #" + actorIndex);

            var currentAgentID = actor.agentID;
            this._clearAgent(actorIndex);
            printDebug("fired actor found #" + actorIndex);

            if (currentAgentID != INVALID_ACTOR) {
                printDebug("fired actor is still connected, send fire command");
                this.sendCommand(currentAgentID, MASTER_FIRE_AGENT);
            }
        }
    }

    MasterController.prototype._clearAgent = function(actorIndex) {
        // check to see if we know about this agent  
        if (actorIndex >= 0) {
            printDebug("before _clearAgent #" + actorIndex + " " + JSON.stringify(this))
            this.knownAgents.splice(actorIndex, 1);
            var lostActor = this.hiredActors[actorIndex];
            this.hiredActors.splice(actorIndex, 1);
            lostActor.agentID = INVALID_ACTOR;
            printDebug("Clearing agent  " + actorIndex + " Forgeting about it");
            printDebug("after _clearAgent #" + actorIndex + " " + JSON.stringify(this))
            
        }
    }

    this.MasterController = MasterController;

    // agent side
    //---------------------------------
    var AgentController = function() {
        this.subscribed = false; 
        
        this._init();

        this.onHired = function() {};
        this.onCommand = function(command) {};
        this.onFired = function() {};
    };

    AgentController.prototype._init = function() {
        this.actorIndex= INVALID_ACTOR; 
        this.notifyAlive = false;
        this.timeSinceLastAlive = 0;
        this.numCyclesWithoutAlive = 0;
        this.actorUUID = makeUniqueUUID(Agent.sessionUUID);
        printDebug("this.actorUUID = " + this.actorUUID);
    }

    AgentController.prototype.destroy = function() {
        if (this.subscribed) {  
            this.fired();    
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
        var announce = unpackAnnounceMessage(message);
        //printDebug("Client " + this.actorIndex + " Received Announcement = " + message);
        if (announce.dest == this.actorUUID) {
            if (announce.command != AGENT_READY) {
                // this may be a message to hire me if i m not already
                if (this.actorIndex == INVALID_ACTOR) {
                    printDebug(announce.command);

                    var parts = announce.command.split(".");
                    var commandPart0 = parts[0];
                    var commandPart1 = parts[1];
                    //printDebug("Client " + Agent.sessionUUID + " - " + agentID + " Hired!");
                //    if (agentID == Agent.sessionUUID) {
                        this.actorIndex = commandPart1;
                        printDebug("Client " + this.actorIndex + " Hired!");
                        this.onHired();         
                    //    Messages.unsubscribe(ANNOUNCE_CHANNEL); // id announce channel
                  //  }
                }
            }
        }
    }

    AgentController.prototype._processCommandMessage = function(message, senderID) {            
        var command = unpackCommandMessage(message);
        //printDebug("Received command = " + JSON.stringify(command));

        if ((command.dest_key == this.actorUUID) || (command.dest_key == AGENTS_BROADCAST)) {
            if (command.action_key == MASTER_ALIVE) {
                this.notifyAlive = true;
            } else if (command.action_key == MASTER_FIRE_AGENT) {
                printDebug("Master firing Agent");
                this.fired();
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
                    Messages.sendMessage(ANNOUNCE_CHANNEL, packAnnounceMessage(MASTER_ID, AGENT_READY, this.actorUUID));
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

        // Post a last message to master in case it still listen to warn that this agent is losing it
        if (wasHired) {
            Messages.sendMessage(ANNOUNCE_CHANNEL, packAnnounceMessage(MASTER_ID, AGENT_LOST, this.actorUUID));
        }

        // reset
        this._init();

        // then custom fire if was hired
        if (wasHired) {
            this.onFired();
        }           
    }


    this.AgentController = AgentController;
})();



