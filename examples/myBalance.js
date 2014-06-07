//
//  myBalance.js
//  examples
//
//  Created by Stojce Slavkovski on June 5, 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Show wallet balance
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var Controller = Controller || {};
var Overlays = Overlays || {};
var Script = Script || {};
var AccountManager = AccountManager || {};

(function () {
    "use strict";
    var iconUrl = 'http://localhost/~stojce/',
        overlayWidth = 150,
        overlayHeight = 150,
        redColor = {
            red: 255,
            green: 0,
            blue: 0
        },
        greenColor = {
            red: 0,
            green: 255,
            blue: 0
        },
        whiteColor = {
            red: 255,
            green: 255,
            blue: 255
        },
        balance = 0,
        voxelTool = Overlays.addOverlay("image", {
            x: 0,
            y: 0,
            width: 50,
            height: 50,
            subImage: {
                x: 0,
                y: 50,
                width: 50,
                height: 50
            },
            imageURL: iconUrl + "wallet.svg",
            alpha: 1
        }),
        textOverlay = Overlays.addOverlay("text", {
            x: 0,
            y: 0,
            width: 55,
            height: 13,
            topMargin: 5,
            text: balance,
            alpha: 0
        });

    function scriptEnding() {
        Overlays.deleteOverlay(voxelTool);
        Overlays.deleteOverlay(textOverlay);
    }

    function update(deltaTime) {
        var xPos = Controller.getViewportDimensions().x;
        Overlays.editOverlay(voxelTool, {
            x: xPos - 150
        });

        Overlays.editOverlay(textOverlay, {
            x: xPos - 100
        });
    }

    function updateBalance(newBalance) {
        if (balance === newBalance) {
            return;
        }

        var change = balance - newBalance,
            textColor = change > 0 ? redColor : greenColor;

        balance = newBalance;
        Overlays.editOverlay(textOverlay, {
            text: balance,
            color: textColor
        });

        Script.setTimeout(function () {
            Overlays.editOverlay(textOverlay, {
                color: whiteColor
            });
        }, 1000);
    }

    AccountManager.balanceChanged.connect(updateBalance);
    Script.scriptEnding.connect(scriptEnding);
    Script.update.connect(update);
}());