"use strict";

//
//  spawnBalloons.js
//
//  Created by Johnathan Franck on 3 May 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var RED_BALLOON_URL = "http://mpassets.highfidelity.com/8410ef73-9506-4dc7-b364-0174998a859e-v1/Models/redBalloon.fbx";
    var BLUE_BALLOON_URL = "http://mpassets.highfidelity.com/8410ef73-9506-4dc7-b364-0174998a859e-v1/Models/blueBalloon.fbx";
    var GREEN_BALLOON_URL = "http://mpassets.highfidelity.com/8410ef73-9506-4dc7-b364-0174998a859e-v1/Models/greenBalloon.fbx";
    var YELLOW_BALLOON_URL = "http://mpassets.highfidelity.com/8410ef73-9506-4dc7-b364-0174998a859e-v1/Models/yellowBalloon.fbx";
    var ORANGE_BALLOON_URL = "http://mpassets.highfidelity.com/8410ef73-9506-4dc7-b364-0174998a859e-v1/Models/orangeBalloon.fbx";
    var CYAN_BALLOON_URL = "http://mpassets.highfidelity.com/8410ef73-9506-4dc7-b364-0174998a859e-v1/Models/cyanBalloon.fbx";
    var PURPLE_BALLOON_URL = "http://mpassets.highfidelity.com/8410ef73-9506-4dc7-b364-0174998a859e-v1/Models/purpleBalloon.fbx";
    //'Blue Skies' by Silent Partner from youtube audio library. Listed as attribution not required
    var SPAWN_MUSIC_URL = "http://mpassets.highfidelity.com/8410ef73-9506-4dc7-b364-0174998a859e-v1/Audio/Blue_Skies.wav"

    var BALLOON_COLORS = ["red", "blue", "green", "yellow", "orange", "cyan", "purple"];
    var BALLOON_URLS = [RED_BALLOON_URL, BLUE_BALLOON_URL, GREEN_BALLOON_URL,
        YELLOW_BALLOON_URL, ORANGE_BALLOON_URL, CYAN_BALLOON_URL, PURPLE_BALLOON_URL];

    var NUM_COLORS = 7;
    var COUNTDOWN_SECONDS = 9;

    var spawnRate = 2000;
    var xRangeMax = 1;
    var zRangeMax = 1;
    var gravityCoefficient = 0.25;
    var balloonLifetime = 10;
    var spawnTime = 300;
    var musicInjector;
    var spawnIntervalID;
    var spawnMusic;
    var countdownIntervalID;
    var countdownEntityID;
    

    this.preload = function(pEntityID) {
        var parentProperties = Entities.getEntityProperties(pEntityID, ["userData"]),
            spawnMusicURL,
            spawnerSettings;
        if (parentProperties.userData){
            spawnerSettings = JSON.parse(parentProperties.userData);
        }
        spawnMusicURL = spawnerSettings.spawnMusicURL ? spawnerSettings.spawnMusicURL : SPAWN_MUSIC_URL;
        spawnMusic = SoundCache.getSound(spawnMusicURL);

        startCountdown(pEntityID);
    };

    function startCountdown(pEntityID) {
        var countdownSeconds = COUNTDOWN_SECONDS,
            parentProperties = Entities.getEntityProperties(pEntityID, ["position"]),
            countdownEntityProperties;

        countdownEntityProperties = {
            type: "Text",
            text: countdownSeconds,
            lineHeight: 0.71,
            dimensions: {
                x: 0.5,
                y: 1,
                z: 0.01
            },
            textColor: {
                red: 0,
                blue: 255,
                green: 0
            },
            backgroundColor: {
                red: 255,
                blue: 255,
                green: 255
            },
            parentID: pEntityID,
            position: parentProperties.position
        };
        
        countdownEntityID = Entities.addEntity(countdownEntityProperties);
        countdownIntervalID = Script.setInterval(function () {
            countdownSeconds -= 1;
            if (countdownSeconds < 0) {
                Script.clearInterval(countdownIntervalID);
                Entities.deleteEntity(countdownEntityID);
            } else {
                Entities.editEntity(countdownEntityID, {"text": countdownSeconds});
                if (countdownSeconds === 0) {
                    spawnBalloons(pEntityID);
                }
            }
        }, 1000);
    }

    function spawnBalloons(pEntityID) {
        var parentProperties = Entities.getEntityProperties(pEntityID, ["position", "userData"]),
            spawnerSettings,
            spawnMusicVolume,
            spawnCount = 0;

        if (parentProperties.userData){
            spawnerSettings = JSON.parse(parentProperties.userData);
        }

        xRangeMax = !isNaN(spawnerSettings.xRangeMax) ? spawnerSettings.xRangeMax : xRangeMax;
        zRangeMax = !isNaN(spawnerSettings.zRangeMax) ? spawnerSettings.zRangeMax : zRangeMax;
        gravityCoefficient = !isNaN(spawnerSettings.gravityCoefficient) ? spawnerSettings.gravityCoefficient : gravityCoefficient;
        spawnTime = !isNaN(spawnerSettings.spawnTime) ? spawnerSettings.spawnTime : spawnTime;
        balloonLifetime = !isNaN(spawnerSettings.balloonLifetime) ? spawnerSettings.balloonLifetime : balloonLifetime;
        spawnRate =  !isNaN(spawnerSettings.spawnRate) ? spawnerSettings.spawnRate : spawnRate;
        spawnMusicVolume = !isNaN(spawnerSettings.spawnMusicVolume) ? spawnerSettings.spawnMusicVolume : 0.1;
        
        if (spawnMusic.downloaded){
            musicInjector = Audio.playSound(spawnMusic, {
                position: parentProperties.position,
                volume: spawnMusicVolume,
                loop: true
            });
        }

        spawnIntervalID = Script.setInterval(function() {
            var colorID = Math.floor(Math.random() * NUM_COLORS),
                color = BALLOON_COLORS[colorID],
                balloonURL = BALLOON_URLS[colorID],
                balloonPosition = {},
                balloonProperties;            
            
            spawnCount ++;
            //Randomize balloon spawn position
            balloonPosition.y = parentProperties.position.y + 0.5;
            balloonPosition.x = parentProperties.position.x + (Math.random() - 0.5) * 2 * xRangeMax;
            balloonPosition.z = parentProperties.position.z + (Math.random() - 0.5) * 2 * zRangeMax;

            balloonProperties = {
                position: balloonPosition,
                lifetime: balloonLifetime,
                angularVelocity: {
                    x: -0.03654664754867554,
                    y: -0.4030083637684583664 + Math.random(),
                    z: 0.02576472796499729
                },
                collisionsWillMove: 1,
                density: 100,
                description: "A happy " + color + " balloon",
                dimensions: {
                  x: 0.3074322044849396,
                  y: 0.40930506587028503,
                  z: 0.30704551935195923
                },
                dynamic: 1,
                gravity: {
                  x: 0,
                  y: gravityCoefficient,
                  z: 0
                },
                modelURL: balloonURL,
                name: color + " balloon",
                queryAACube: {
                  scale: 0.596927285194397,
                  x: -0.2984636425971985,
                  y: -0.2984636425971985,
                  z: -0.2984636425971985
                },
                restitution: 0.9900000095367432,
                rotation: {
                  w: -0.10368101298809052,
                  x: 0.5171623826026917,
                  y: 0.1211432576179504,
                  z: -0.670971691608429 + Math.random()
                },
                shapeType: "sphere",
                type : "Model",
                velocity: {
                  x: 0.003623033408075571,
                  y: 0.0005839366931468248,
                  z: -0.01512028019875288
                }
            };
            Entities.addEntity(balloonProperties);

            //Stop spawning after spawnTime
            if (spawnCount * spawnRate / 1000 > spawnTime){
                Script.clearInterval(spawnIntervalID);
                 if (musicInjector !== undefined && musicInjector.isPlaying) {
                    musicInjector.stop();
                    musicInjector = undefined;
                }	
            }
         }, spawnRate);
    }	

    this.unload = function(){
        if (spawnIntervalID){
            Script.clearInterval(spawnIntervalID);
        }
        if (countdownIntervalID){
            Script.clearInterval(countdownIntervalID);
        }
        if (countdownEntityID){
            Entities.deleteEntity(countdownEntityID);
        }
        if (musicInjector !== undefined && musicInjector.isPlaying) {
            musicInjector.stop();
            musicInjector = undefined;
        }		
    };
    
});
