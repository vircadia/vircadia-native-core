//
//  myBalance.js
//  examples
//
//  Created by Stojce Slavkovski on June 5, 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Show wallet ₵ balance
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("libraries/globals.js");

var Controller = Controller || {};
var Overlays = Overlays || {};
var Script = Script || {};
var Account = Account || {};

(function () {
    "use strict";
    var iconUrl = HIFI_PUBLIC_BUCKET + 'images/tools/',
        overlayWidth = 150,
        overlayHeight = 50,
        overlayTopOffset = 15,
        overlayRightOffset = 140,
        textRightOffset = 105,
        maxIntegers = 5,
        downColor = {
            red: 0,
            green: 0,
            blue: 255
        },
        upColor = {
            red: 0,
            green: 255,
            blue: 0
        },
        normalColor = {
            red: 204,
            green: 204,
            blue: 204
        },
        balance = -1,
        walletBox = Overlays.addOverlay("image", {
            x: 0,
            y: overlayTopOffset,
            width: 122,
            height: 32,
            imageURL: iconUrl + "wallet.svg",
            alpha: 1
        }),
        textOverlay = Overlays.addOverlay("text", {
            x: 0,
            y: overlayTopOffset,
            topMargin: 10,
            font: {
                size: 16
            },
            color: normalColor
        });

    function scriptEnding() {
        Overlays.deleteOverlay(walletBox);
        Overlays.deleteOverlay(textOverlay);
    }

    function update(deltaTime) {
        var xPos = Controller.getViewportDimensions().x;
        Overlays.editOverlay(walletBox, {
            x: xPos - overlayRightOffset,
            visible: Account.isLoggedIn()
        });

        Overlays.editOverlay(textOverlay, {
            x: xPos - textRightOffset,
            visible: Account.isLoggedIn()
        });
    }

    function formatedBalance() {
        var integers = balance.toFixed(0).length,
            decimals = Math.abs(maxIntegers - integers) + 2;

        var x = balance.toFixed(decimals).split('.'),
            x1 = x[0],
            x2 = x.length > 1 ? '.' + x[1] : '';
        var rgx = /(\d+)(\d{3})/;
        while (rgx.test(x1)) {
            x1 = x1.replace(rgx, '$1' + ',' + '$2');
        }
        return x1 + x2;
    }

    function updateBalance(newBalance) {
        if (balance === newBalance) {
            return;
        }

        var change = newBalance - balance,
            textColor = change < 0 ? downColor : upColor;

        balance = newBalance;
        Overlays.editOverlay(textOverlay, {
            text: formatedBalance(),
            color: textColor
        });

        Script.setTimeout(function () {
            Overlays.editOverlay(textOverlay, {
                color: normalColor
            });
        }, 1000);
    }

    updateBalance(Account.getBalance());
    Account.balanceChanged.connect(updateBalance);
    Script.scriptEnding.connect(scriptEnding);
    Script.update.connect(update);
}());