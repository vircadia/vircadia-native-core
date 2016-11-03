//
//  spinButtons.js
//
//  Created by David Rowe on 20 Apr 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function hoverSpinButtons(event) {
    var input = event.target,
        x = event.offsetX,
        y = event.offsetY,
        width = input.offsetWidth,
        height = input.offsetHeight,
        SPIN_WIDTH = 11,
        SPIN_MARGIN = 2,
        maxX = width - SPIN_MARGIN,
        minX = maxX - SPIN_WIDTH;

    if (minX <= x && x <= maxX) {
        if (y < height / 2) {
            input.classList.remove("hover-down");
            input.classList.add("hover-up");
        } else {
            input.classList.remove("hover-up");
            input.classList.add("hover-down");
        }
    } else {
        input.classList.remove("hover-up");
        input.classList.remove("hover-down");
    }
}

function unhoverSpinButtons(event) {
    event.target.classList.remove("hover-up");
    event.target.classList.remove("hover-down");
}

function augmentSpinButtons() {
    var inputs, i, length;

    inputs = document.getElementsByTagName("INPUT");
    for (i = 0, length = inputs.length; i < length; i += 1) {
        if (inputs[i].type === "number") {
            inputs[i].addEventListener("mousemove", hoverSpinButtons);
            inputs[i].addEventListener("mouseout", unhoverSpinButtons);
        }
    }
}
