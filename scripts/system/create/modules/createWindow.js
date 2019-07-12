"use strict";

//  createWindow.js
//
//  Created by Thijs Wenker on 6/1/18
//
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var getWindowRect = function(settingsKey, defaultRect) {
    var windowRect = Settings.getValue(settingsKey, defaultRect);
    return windowRect;
};

var setWindowRect = function(settingsKey, position, size) {
    Settings.setValue(settingsKey, {
        position: position,
        size: size
    });
};

var CallableEvent = (function() {
    function CallableEvent() {
        this.callbacks = [];
    }

    CallableEvent.prototype = {
        callbacks: null,
        call: function () {
            var callArguments = arguments;
            this.callbacks.forEach(function(callbackObject) {
                try {
                    callbackObject.callback.apply(callbackObject.context ? callbackObject.context : this, callArguments);
                } catch (e) {
                    console.error('Call to CallableEvent callback failed!');
                }
            });
        },
        addListener: function(contextOrCallback, callback) {
            if (callback) {
                this.callbacks.push({
                    context: contextOrCallback,
                    callback: callback
                });
            } else {
                this.callbacks.push({
                    callback: contextOrCallback
                });
            }
        },
        removeListener: function(callback) {
            var foundIndex = -1;
            this.callbacks.forEach(function (callbackObject, index) {
                if (callbackObject.callback === callback) {
                    foundIndex = index;
                }
            });

            if (foundIndex !== -1) {
                this.callbacks.splice(foundIndex, 1);
            }
        }
    };

    return CallableEvent;
})();

module.exports = (function() {
    function CreateWindow(qmlPath, title, settingsKey, defaultRect, createOnStartup) {
        this.qmlPath = qmlPath;
        this.title = title;
        this.settingsKey = settingsKey;
        this.defaultRect = defaultRect;
        this.webEventReceived = new CallableEvent();
        this.interactiveWindowHidden = new CallableEvent();
        this.fromQml = new CallableEvent();
        if (createOnStartup) {
            this.createWindow();
        }
    }

    CreateWindow.prototype = {
        window: null,
        createWindow: function() {
            var defaultRect = this.defaultRect;
            if (typeof this.defaultRect === "function") {
                defaultRect = this.defaultRect();
            }

            var windowRect = getWindowRect(this.settingsKey, defaultRect);
            this.window = Desktop.createWindow(this.qmlPath, {
                title: this.title,
                additionalFlags: Desktop.ALWAYS_ON_TOP | Desktop.CLOSE_BUTTON_HIDES,
                presentationMode: Desktop.PresentationMode.NATIVE,
                size: windowRect.size,
                visible: true,
                position: windowRect.position
            });

            var windowRectChanged = function () {
                if (this.window.visible) {
                    setWindowRect(this.settingsKey, this.window.position, this.window.size);
                }
            };

            this.window.sizeChanged.connect(this, windowRectChanged);
            this.window.positionChanged.connect(this, windowRectChanged);

            this.window.webEventReceived.connect(this, function(data) {
                this.webEventReceived.call(data);
            });

            this.window.visibleChanged.connect(this, function() {
                if (!this.window.visible) {
                    this.interactiveWindowHidden.call();
                }
            });

            this.window.fromQml.connect(this, function (data) {
                this.fromQml.call(data);
            });

            Script.scriptEnding.connect(this, function() {
                this.window.close();
            });
        },
        setVisible: function(visible) {
            if (visible && !this.window) {
                this.createWindow();
            }

            if (this.window) {
                if (visible) {
                    this.window.show();
                } else {
                    this.window.visible = false;
                }
            }
        },
        isVisible: function() {
            if (this.window) {
                return this.window.visible;
            }
            return false;
        },
        emitScriptEvent: function(data) {
            if (this.window) {
                this.window.emitScriptEvent(data);
            }
        },
        sendToQml: function(data) {
            if (this.window) {
                this.window.sendToQml(data);
            }
        },
        webEventReceived: null,
        interactiveWindowHidden: null,
        fromQml: null
    };

    return CreateWindow;
})();
