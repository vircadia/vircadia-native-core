//  draggableNumber.js
//
//  Created by David Back on 7 Nov 2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

debugPrint = function (message) {
    console.log(message);
};

function DraggableNumber(min, max, step) {
    this.min = min;
    this.max = max;
    this.step = step !== undefined ? step : 1;
    this.startEvent = null;
    this.initialize();
}

DraggableNumber.prototype = {
    onMouseDown: function(event) {
        let that = this.draggableNumber;
        that.mouseDown(event);
    },
    
    mouseDown: function(event) {
        this.startEvent = event;
        document.addEventListener("mousemove", this.onMouseMove);
        document.addEventListener("mouseup", this.onMouseUp);
    },
    
    mouseMove: function(event, that) {
        if (this.startEvent) {
            let dx = event.clientX - this.startEvent.clientX;
            let valueChange = dx * this.step;
            let newValue = parseFloat(this.elInput.value) + valueChange;
            if (this.min !== undefined && newValue < this.min) {
                newValue = this.min;
            } else if (this.max !== undefined && newValue > this.max) {
                newValue = this.max;
            }
            this.elInput.value = newValue;
            this.inputChangeFunction();
            this.startEvent = event;
        }
    },
    
    mouseUp: function(event) {
        this.startEvent = null;
        document.removeEventListener("mousemove", this.onMouseMove);
        document.removeEventListener("mouseup", this.onMouseUp);
    },
    
    setInputChangeFunction: function(inputChangeFunction) {
        this.inputChangeFunction = inputChangeFunction.bind(this.elInput);
        this.elInput.addEventListener('change', this.inputChangeFunction);
    },
    
    initialize: function() {
        this.onMouseMove = this.mouseMove.bind(this);
        this.onMouseUp = this.mouseUp.bind(this);
        
        this.elDiv = document.createElement('div');
        this.elDiv.className = "draggable-number";
        
        this.elInput = document.createElement('input');
        this.elInput.setAttribute("type", "number");
        if (this.min !== undefined) {
            this.elInput.setAttribute("min", this.min);
        }
        if (this.max !== undefined) {
            this.elInput.setAttribute("max", this.max);
        }
        if (this.step !== undefined) {
            this.elInput.setAttribute("step", this.step);
        }
        this.elInput.draggableNumber = this;
        this.elInput.addEventListener("mousedown", this.onMouseDown);

        this.elLeftArrow = document.createElement('span');
        this.elRightArrow = document.createElement('span');
        this.elLeftArrow.className = 'draggable-number left-arrow';
        this.elLeftArrow.innerHTML = 'D';
        this.elRightArrow.className = 'draggable-number right-arrow';
        this.elRightArrow.innerHTML = 'D';
        
        this.elDiv.appendChild(this.elLeftArrow);
        this.elDiv.appendChild(this.elInput);
        this.elDiv.appendChild(this.elRightArrow);
    }
};
