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
    var SERVICE_CHANNEL = "com.highfidelity.playback.service";
    var COMMAND_CHANNEL = "com.highfidelity.playback.command";

    // The time between alive messages on the command channel
    var ALIVE_PERIOD = 3;
    var NUM_CYCLES_BEFORE_RESET = 8;

    // Service Actions
    var MASTER_ID = -1;
    var INVALID_AGENT = -2;

    var BROADCAST_AGENTS = -3;

    var MASTER_ALIVE = "MASTER_ALIVE";
    var AGENT_ALIVE = "AGENT_ALIVE";
    var AGENT_READY = "READY";
    var MASTER_HIRE_AGENT = "HIRE"
    var MASTER_FIRE_AGENT = "FIRE";

    var makeUniqueUUID = function(SEUUID) {
        //return SEUUID + Math.random();
        // forget complexity, just give me a four digit pin
        return (Math.random() * 10000).toFixed(0);
    }

    var packServiceMessage = function(dest, command, src) {
        var message = {
            dest: dest,
            command: command,
            src: src
        };
        return JSON.stringify(message);
    };

    var unpackServiceMessage = function(message) {
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
        this.agentID = INVALID_AGENT;
        this.lastAliveCycle = 0;  
        this.onHired = function(actor) {};
        this.onFired = function(actor) {};
    };

    Actor.prototype.isConnected = function () {
        return (this.agentID != INVALID_AGENT);
    }

    Actor.prototype.alive = function () {
        printDebug("Agent UUID =" + this.agentID + " Alive was " + this.lastAliveCycle);
        this.lastAliveCycle = 0; 
    }
    Actor.prototype.incrementAliveCycle = function () {
        printDebug("Actor.prototype.incrementAliveCycle UUID =" + this.agentID + " Alive pre increment " + this.lastAliveCycle);
        if (this.isConnected()) {
            this.lastAliveCycle++; 
            printDebug("Agent UUID =" + this.agentID + " Alive incremented " + this.lastAliveCycle);
        }
        return this.lastAliveCycle;
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
            Messages.unsubscribe(SERVICE_CHANNEL);
            Messages.unsubscribe(COMMAND_CHANNEL);
            this.subscribed = true;
        }
    };

    MasterController.prototype.reset = function() {
        this.timeSinceLastAlive = 0;

        if (!this.subscribed) {
            Messages.subscribe(COMMAND_CHANNEL);
            Messages.subscribe(SERVICE_CHANNEL);
            var localThis = this;
            Messages.messageReceived.connect(function (channel, message, senderID) {
                if (channel == SERVICE_CHANNEL) {
                    localThis._processServiceMessage(message, senderID);
                    return;
                }  
            });
        }
        // ready to roll, enable
        this.subscribed = true;
        printDebug("Master Started");
    };

    MasterController.prototype._processServiceMessage = function(message, senderID) {
        var message = unpackServiceMessage(message);       
        if (message.dest == MASTER_ID) {
            if (message.command == AGENT_READY) {
                // check to see if we know about this agent
                var agentIndex = this.knownAgents.indexOf(message.src);
                if (agentIndex < 0) {
                    this._onAgentAvailableForHiring(message.src);
                } else {
                    // Master think the agent is hired but not the other way around, forget about it
                    printDebug("New agent still sending ready ? " + message.src + " " + agentIndex + " Forgeting about it");
                    this._removeAgent(agentIndex);
                }
            } else if (message.command == AGENT_ALIVE) {
                 // check to see if we know about this agent
                var agentIndex = this.knownAgents.indexOf(message.src);
                if (agentIndex >= 0) {
                    // yes so reset its alive beat
                    this.hiredActors[agentIndex].alive();                  
                    return;
                } else {
                    return;
                }
            }
        }
    };

    MasterController.prototype._onAgentAvailableForHiring = function(agentID) {
        if (this.hiringAgentsQueue.length == 0) {
            printDebug("No Actor on the hiring queue");
            return;
        }

        printDebug("MasterController.prototype._onAgentAvailableForHiring " + agentID);
        var newActor = this.hiringAgentsQueue.pop();

        var indexOfNewAgent = this.knownAgents.push(agentID);
        newActor.alive();
        newActor.agentID = agentID;    
        this.hiredActors.push(newActor);
                          
        printDebug("New agent available to be hired " + agentID + " " + indexOfNewAgent);                        
        Messages.sendMessage(SERVICE_CHANNEL, packServiceMessage(agentID, MASTER_HIRE_AGENT, MASTER_ID));
        printDebug("message sent calling the actor" + JSON.stringify(newActor) );

        newActor.onHired(newActor); 
    }

     MasterController.prototype.sendCommand = function(target, action, argument) {      
        if (this.subscribed) {
            var command = packCommandMessage(target, action, argument);
            printDebug(command);         
            Messages.sendMessage(COMMAND_CHANNEL, command);
        }
    };

    MasterController.prototype.update = function(deltaTime) {
        this.timeSinceLastAlive += deltaTime;
        if (this.timeSinceLastAlive > ALIVE_PERIOD) {
            this.timeSinceLastAlive = 0;
            Messages.sendMessage(SERVICE_CHANNEL, packServiceMessage(BROADCAST_AGENTS, MASTER_ALIVE, MASTER_ID));
        
            printDebug("checking the agents status");
            {
                // Check for alive connected agents
                var lostAgents = new Array();
                for (var i = 0; i < this.hiredActors.length; i++) {
                    var actor = this.hiredActors[i];
                    printDebug("checking :" + JSON.stringify(actor));
                    var lastAlive = actor.incrementAliveCycle()
                    if (lastAlive > NUM_CYCLES_BEFORE_RESET) {
                        printDebug("Agent Lost, firing Agent");
                        lostAgents.push(actor);
                    }
                }

                // now fire gathered lost agents
                if (lostAgents.length > 0) {
                    printDebug("Firing " + lostAgents.length + " agents" + JSON.stringify(lostAgents));
                        
                    for (var i = 0; i < lostAgents.length; i++) {
                        this.fireAgent(lostAgents[l]);
                    }
                }
            }
        }
    };


    MasterController.prototype.hireAgent = function(actor) {
        if (actor == null) {
            printDebug("trying to hire an agent with a null actor, abort");
            return;
        }
        if (actor.isConnected()) {
            printDebug("trying to hire an agent already connected, abort");
            return;
        }
        this.hiringAgentsQueue.unshift(actor); 
    };

    MasterController.prototype.fireAgent = function(actor) {
        // check to see if we know about this agent
        printDebug("MasterController.prototype.fireAgent" + JSON.stringify(actor) + "  " + JSON.stringify(this.knownAgents));
        
        var waitingIndex = this.hiringAgentsQueue.indexOf(actor);                
        if (waitingIndex >= 0) {
            printDebug("MasterController.prototype.fireAgent found actor on waiting queue #" + waitingIndex);
            this.hiringAgentsQueue.splice(waitingIndex, 1);
        }

        var actorIndex = this.knownAgents.indexOf(actor.agentID);                
        if (actorIndex >= 0) {
            printDebug("fired actor found #" + actorIndex);
            this._removeAgent(actorIndex);
        }
    }

    MasterController.prototype._removeAgent = function(actorIndex) {
        // check to see if we know about this agent  
        if (actorIndex >= 0) {
            printDebug("before _removeAgent #" + actorIndex + " " + JSON.stringify(this))
            this.knownAgents.splice(actorIndex, 1);

            var lostActor = this.hiredActors[actorIndex];
            this.hiredActors.splice(actorIndex, 1);
            
            var lostAgentID = lostActor.agentID;
            lostActor.agentID = INVALID_AGENT;

            printDebug("Clearing agent  " + actorIndex + " Forgeting about it");
            printDebug("after _removeAgent #" + actorIndex + " " + JSON.stringify(this))
        
            if (lostAgentID != INVALID_AGENT) {
                printDebug("fired actor is still connected, send fire command");
                Messages.sendMessage(SERVICE_CHANNEL, packServiceMessage(lostAgentID, MASTER_FIRE_AGENT, MASTER_ID));
            }
            
            lostActor.onFired(lostActor);    
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
        this.isHired= false; 
        this.timeSinceLastAlive = 0;
        this.numCyclesWithoutAlive = 0;
        this.agentUUID = makeUniqueUUID(Agent.sessionUUID);
        printDebug("this.agentUUID = " + this.agentUUID);
    }

    AgentController.prototype.destroy = function() {
        if (this.subscribed) {  
            this.fired();    
            Messages.unsubscribe(SERVICE_CHANNEL);
            Messages.unsubscribe(COMMAND_CHANNEL);
            this.subscribed = true;
        }
    };

    AgentController.prototype.reset = function() {
        // If already hired, fire
        this.fired();

        if (!this.subscribed) {
            Messages.subscribe(COMMAND_CHANNEL);
            Messages.subscribe(SERVICE_CHANNEL);
            var localThis = this;
            Messages.messageReceived.connect(function (channel, message, senderID) {
                if (channel == SERVICE_CHANNEL) {
                    localThis._processServiceMessage(message, senderID);
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

    AgentController.prototype._processServiceMessage = function(message, senderID) {
        var announce = unpackServiceMessage(message);
        //printDebug("Client " + this.agentUUID + " Received Announcement = " + message);
        if (announce.dest == this.agentUUID) {
            if (announce.command != AGENT_READY) {
                var parts = announce.command.split(".");
                    
                // this is potnetially a message to hire me if i m not already
                if (!this.isHired && (parts[0] == MASTER_HIRE_AGENT)) {
                    printDebug(announce.command);
                    this.isHired = true;
                    printDebug("Client Hired by master UUID" + senderID);
                    this.onHired();         
                    return;
                }

                if (this.isHired && (parts[0] == MASTER_FIRE_AGENT)) {
                    printDebug("Client Fired by master UUID" + senderID);
                    this.fired();
                    return;   
                }
            }
        } else if (announce.src == MASTER_ID && announce.command ==  MASTER_ALIVE) {
            this.numCyclesWithoutAlive = 0;
            return;
        }
    }

    AgentController.prototype._processCommandMessage = function(message, senderID) {            
        var command = unpackCommandMessage(message);
        if ((command.dest_key == this.agentUUID) || (command.dest_key == BROADCAST_AGENTS)) {
            printDebug("Command received = " + JSON.stringify(command) + senderID); 
            this.onCommand(command);
        } else {
            // ignored
        }
    };


    AgentController.prototype.update = function(deltaTime) {
        this.timeSinceLastAlive += deltaTime;
        if (this.timeSinceLastAlive > ALIVE_PERIOD) {
            if (this.subscribed) {
                if (!this.isHired) {
                    Messages.sendMessage(SERVICE_CHANNEL, packServiceMessage(MASTER_ID, AGENT_READY, this.agentUUID));
                    //printDebug("Client Ready" + SERVICE_CHANNEL + AGENT_READY); 
                } else {
                    
                    // Send alive beat
                    Messages.sendMessage(SERVICE_CHANNEL, packServiceMessage(MASTER_ID, AGENT_ALIVE, this.agentUUID));
                    
                    // Listen for master beat
                    this.numCyclesWithoutAlive++;
                    if (this.numCyclesWithoutAlive > NUM_CYCLES_BEFORE_RESET) {
                        printDebug("Master Lost, self firing Agent");
                        this.fired();           
                    }
                }
            }

            this.timeSinceLastAlive = 0;    
        }
    };

    AgentController.prototype.fired = function() {
        // clear the state first
        var wasHired = this.isHired;

        // reset
        this._init();

        // then custom fire if was hired
        if (wasHired) {
            this.onFired();
        }           
    }


    this.AgentController = AgentController;



    this.BROADCAST_AGENTS = BROADCAST_AGENTS;
})();



