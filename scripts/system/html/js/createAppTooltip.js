//  createAppTooltip.js
//
//  Created by Thijs Wenker on 17 Oct 2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

const CREATE_APP_TOOLTIP_OFFSET = 20;

function CreateAppTooltip() {
    this._tooltipData = null;
    this._tooltipDiv = null;
}

CreateAppTooltip.prototype = {
    _tooltipData: null,
    _tooltipDiv: null,

    _removeTooltipIfExists: function() {
        if (this._tooltipDiv !== null) {
            this._tooltipDiv.remove();
            this._tooltipDiv = null;
        }
    },

    setTooltipData: function(tooltipData) {
        this._tooltipData = tooltipData;
    },

    registerTooltipElement: function(element, tooltipID) {
        element.addEventListener("mouseover", function() {

            this._removeTooltipIfExists();

            let tooltipData = this._tooltipData[tooltipID];

            if (!tooltipData || tooltipData.tooltip === "") {
                return;
            }

            let elementRect = element.getBoundingClientRect();
            let elTip = document.createElement("div");
            elTip.className = "createAppTooltip";

            let elTipDescription = document.createElement("div");
            elTipDescription.className = "createAppTooltipDescription";
            elTipDescription.innerText = tooltipData.tooltip;
            elTip.appendChild(elTipDescription);

            let jsAttribute = tooltipID;
            if (tooltipData.jsPropertyName) {
                jsAttribute = tooltipData.jsPropertyName;
            }

            if (!tooltipData.skipJSProperty) {
                let elTipJSAttribute = document.createElement("div");
                elTipJSAttribute.className = "createAppTooltipJSAttribute";
                elTipJSAttribute.innerText = `JS Attribute: ${jsAttribute}`;
                elTip.appendChild(elTipJSAttribute);
            }

            document.body.appendChild(elTip);

            let elementTop = window.pageYOffset + elementRect.top;

            let desiredTooltipTop = elementTop + element.clientHeight + CREATE_APP_TOOLTIP_OFFSET;

            if ((window.innerHeight + window.pageYOffset) < (desiredTooltipTop + elTip.clientHeight)) {
                // show above when otherwise out of bounds
                elTip.style.top = elementTop - CREATE_APP_TOOLTIP_OFFSET - elTip.clientHeight;
            } else {
                // show tooltip on below by default
                elTip.style.top = desiredTooltipTop;
            }
            elTip.style.left = window.pageXOffset + elementRect.left;

            this._tooltipDiv = elTip;
        }.bind(this), false);
        element.addEventListener("mouseout", function() {
            this._removeTooltipIfExists();
        }.bind(this), false);
    }
};
