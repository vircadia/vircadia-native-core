//  createAppTooltip.js
//
//  Created by Thijs Wenker on 17 Oct 2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

const CREATE_APP_TOOLTIP_OFFSET = 20;
const TOOLTIP_DELAY = 500; // ms
const TOOLTIP_DEBUG = false;

function CreateAppTooltip() {
    this._tooltipData = null;
    this._tooltipDiv = null;
    this._delayTimeout = null;
    this._isEnabled = false;
}

CreateAppTooltip.prototype = {
    _tooltipData: null,
    _tooltipDiv: null,
    _delayTimeout: null,
    _isEnabled: null,

    _removeTooltipIfExists: function() {
        if (this._delayTimeout !== null) {
            window.clearTimeout(this._delayTimeout);
            this._delayTimeout = null;
        }

        if (this._tooltipDiv !== null) {
            this._tooltipDiv.remove();
            this._tooltipDiv = null;
        }
    },

    setIsEnabled: function(isEnabled) {
        this._isEnabled = isEnabled;
    },

    setTooltipData: function(tooltipData) {
        this._tooltipData = tooltipData;
    },

    registerTooltipElement: function(element, tooltipID) {
        element.addEventListener("mouseover", function() {
            if (!this._isEnabled) {
                return;
            }

            this._removeTooltipIfExists();

            this._delayTimeout = window.setTimeout(function() {
                let tooltipData = this._tooltipData[tooltipID];

                if (!tooltipData || tooltipData.tooltip === "") {
                    if (!TOOLTIP_DEBUG) {
                        return;
                    }
                    tooltipData = { tooltip: 'PLEASE SET THIS TOOLTIP' };
                }

                let elementRect = element.getBoundingClientRect();
                let elTip = document.createElement("div");
                elTip.className = "create-app-tooltip";

                let elTipDescription = document.createElement("div");
                elTipDescription.className = "create-app-tooltip-description";
                elTipDescription.innerText = tooltipData.tooltip;
                elTip.appendChild(elTipDescription);

                let jsAttribute = tooltipID;
                if (tooltipData.jsPropertyName) {
                    jsAttribute = tooltipData.jsPropertyName;
                }

                if (!tooltipData.skipJSProperty) {
                    let elTipJSAttribute = document.createElement("div");
                    elTipJSAttribute.className = "create-app-tooltip-js-attribute";
                    elTipJSAttribute.innerText = `JS Attribute: ${jsAttribute}`;
                    elTip.appendChild(elTipJSAttribute);
                }

                document.body.appendChild(elTip);

                let elementTop = window.pageYOffset + elementRect.top;

                let desiredTooltipTop = elementTop + element.clientHeight + CREATE_APP_TOOLTIP_OFFSET;
                let desiredTooltipLeft = window.pageXOffset + elementRect.left;

                if ((window.innerHeight + window.pageYOffset) < (desiredTooltipTop + elTip.clientHeight)) {
                    // show above when otherwise out of bounds
                    elTip.style.top = elementTop - CREATE_APP_TOOLTIP_OFFSET - elTip.clientHeight;
                } else {
                    // show tooltip below by default
                    elTip.style.top = desiredTooltipTop;
                }
                if ((window.innerWidth + window.pageXOffset) < (desiredTooltipLeft + elTip.clientWidth)) {
                    elTip.style.left = document.body.clientWidth + window.pageXOffset - elTip.offsetWidth;
                } else {
                    elTip.style.left = desiredTooltipLeft;
                }

                this._tooltipDiv = elTip;
            }.bind(this), TOOLTIP_DELAY);
        }.bind(this), false);
        element.addEventListener("mouseout", function() {
            if (!this._isEnabled) {
                return;
            }

            this._removeTooltipIfExists();
        }.bind(this), false);
    }
};
