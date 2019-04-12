//  draggableNumber.js
//
//  Created by David Back on 7 Nov 2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

const DELTA_X_FOCUS_THRESHOLD = 1;
const ENTER_KEY = 13;

function DraggableNumber(min, max, step, decimals, dragStart, dragEnd) {
    this.min = min;
    this.max = max;
    this.step = step !== undefined ? step : 1;
    this.multiDiffModeEnabled = false;
    this.decimals = decimals;
    this.dragStartFunction = dragStart;
    this.dragEndFunction = dragEnd;
    this.dragging = false;
    this.initialMouseEvent = null;
    this.lastMouseEvent = null;
    this.valueChangeFunction = null;
    this.multiDiffDragFunction = null;
    this.initialize();
}

DraggableNumber.prototype = {
    showInput: function() {
        this.elText.style.visibility = "hidden";
        this.elLeftArrow.style.visibility = "hidden";
        this.elRightArrow.style.visibility = "hidden";
        this.elInput.style.opacity = 1;
    },

    hideInput: function() {
        this.elText.style.visibility = "visible";
        this.elLeftArrow.style.visibility = "visible";
        this.elRightArrow.style.visibility = "visible";
        this.elInput.style.opacity = 0;
    },

    mouseDown: function(event) {
        if (event.target === this.elText && !this.isDisabled()) {
            this.initialMouseEvent = event;
            this.lastMouseEvent = event;
            document.addEventListener("mousemove", this.onDocumentMouseMove);
            document.addEventListener("mouseup", this.onDocumentMouseUp);
        }
    },
    
    mouseUp: function(event) {
        if (!this.dragging && event.target === this.elText && this.initialMouseEvent) {
            let dx = event.clientX - this.initialMouseEvent.clientX;
            if (Math.abs(dx) <= DELTA_X_FOCUS_THRESHOLD) {
                this.showInput();
                this.elInput.focus();
            }
            this.initialMouseEvent = null;
        }
    },
    
    documentMouseMove: function(event) {
        if (!this.dragging && this.initialMouseEvent) {
            let dxFromInitial = event.clientX - this.initialMouseEvent.clientX;
            if (Math.abs(dxFromInitial) > DELTA_X_FOCUS_THRESHOLD) {
                if (this.dragStartFunction) {
                    this.dragStartFunction();
                }
                this.dragging = true;
            }
            this.lastMouseEvent = event;
        }
        if (this.dragging && this.lastMouseEvent) {
            let dragDelta = event.clientX - this.lastMouseEvent.clientX;
            if (dragDelta !== 0) {
                if (this.multiDiffModeEnabled) {
                    if (this.multiDiffDragFunction) {
                        this.multiDiffDragFunction(dragDelta * this.step);
                    }
                } else {
                    if (dragDelta > 0) {
                        this.elInput.stepUp(dragDelta);
                    } else {
                        this.elInput.stepDown(-dragDelta);
                    }
                    this.inputChange();
                    if (this.valueChangeFunction) {
                        this.valueChangeFunction();
                    }
                }
            }
            this.lastMouseEvent = event;
        }
    },
    
    documentMouseUp: function(event) {
        if (this.dragging) {
            if (this.dragEndFunction) {
                this.dragEndFunction();
            }
            this.dragging = false;
        }
        this.lastMouseEvent = null;
        document.removeEventListener("mousemove", this.onDocumentMouseMove);
        document.removeEventListener("mouseup", this.onDocumentMouseUp);
    },
    
    stepUp: function() {
        if (!this.isDisabled()) {
            this.elInput.value = parseFloat(this.elInput.value) + this.step;
            this.inputChange();
            if (this.valueChangeFunction) {
                this.valueChangeFunction();
            }
        }
    },
    
    stepDown: function() {
        if (!this.isDisabled()) {
            this.elInput.value = parseFloat(this.elInput.value) - this.step;
            this.inputChange();
            if (this.valueChangeFunction) {
                this.valueChangeFunction();
            }
        }
    },
    
    setValue: function(newValue, isMultiDiff) {
        if (isMultiDiff !== undefined) {
            this.setMultiDiff(isMultiDiff);
        }

        if (isNaN(newValue)) {
            throw newValue + " is not a number";
        }

        if (newValue !== "" && this.decimals !== undefined) {
            this.elInput.value = parseFloat(newValue).toFixed(this.decimals);
        } else {
            this.elInput.value = newValue;
        }
        this.elText.firstChild.data = this.elInput.value;
    },

    setMultiDiff: function(isMultiDiff) {
        this.multiDiffModeEnabled = isMultiDiff;
        if (isMultiDiff) {
            this.elDiv.classList.add('multi-diff');
        } else {
            this.elDiv.classList.remove('multi-diff');
        }
    },

    setValueChangeFunction: function(valueChangeFunction) {
        this.valueChangeFunction = valueChangeFunction.bind(this.elInput);
        this.elInput.addEventListener("change", this.valueChangeFunction);
    },

    setMultiDiffDragFunction: function(multiDiffDragFunction) {
        this.multiDiffDragFunction = multiDiffDragFunction;
    },

    inputChange: function() {
        let value = this.elInput.value;
        if (this.max !== undefined) {
            value = Math.min(this.max, value);
        }
        if (this.min !== undefined) {
            value = Math.max(this.min, value);
        }
        this.setValue(value);
    },
    
    inputBlur: function(ev) {
        this.hideInput();
    },

    keyPress: function(event) {
        if (event.keyCode === ENTER_KEY) {
            if (this.valueChangeFunction) {
                this.valueChangeFunction();
            }
            this.inputBlur();
        }
    },
    
    isDisabled: function() {
        return this.elText.getAttribute("disabled") === "disabled";
    },
    
    updateMinMax: function(min, max) {
        this.min = min;
        this.max = max;
        if (this.min !== undefined) {
            this.elInput.setAttribute("min", this.min);
        }
        if (this.max !== undefined) {
            this.elInput.setAttribute("max", this.max);
        }
    },
    
    initialize: function() {
        this.onMouseDown = this.mouseDown.bind(this);
        this.onMouseUp = this.mouseUp.bind(this);
        this.onDocumentMouseMove = this.documentMouseMove.bind(this);
        this.onDocumentMouseUp = this.documentMouseUp.bind(this);
        this.onStepUp = this.stepUp.bind(this);
        this.onStepDown = this.stepDown.bind(this);
        this.onInputChange = this.inputChange.bind(this);
        this.onInputBlur = this.inputBlur.bind(this);
        this.onKeyPress = this.keyPress.bind(this);
        
        this.elDiv = document.createElement('div');
        this.elDiv.className = "draggable-number";
        
        this.elText = document.createElement('span');
        this.elText.className = "text";
        this.elText.innerText = " ";
        this.elText.style.visibility = "visible";
        this.elText.addEventListener("mousedown", this.onMouseDown);
        this.elText.addEventListener("mouseup", this.onMouseUp);
        
        this.elLeftArrow = document.createElement('span');
        this.elRightArrow = document.createElement('span');
        this.elLeftArrow.className = 'left-arrow';
        this.elLeftArrow.innerHTML = 'D';
        this.elLeftArrow.addEventListener("click", this.onStepDown);
        this.elRightArrow.className = 'right-arrow';
        this.elRightArrow.innerHTML = 'D';
        this.elRightArrow.addEventListener("click", this.onStepUp);

        this.elMultiDiff = document.createElement('span');
        this.elMultiDiff.className = 'multi-diff';

        this.elInput = document.createElement('input');
        this.elInput.className = "input";
        this.elInput.setAttribute("type", "number");
        this.updateMinMax(this.min, this.max);
        if (this.step !== undefined) {
            this.elInput.setAttribute("step", this.step);
        }
        this.elInput.style.opacity = 0;
        this.elInput.addEventListener("change", this.onInputChange);
        this.elInput.addEventListener("blur", this.onInputBlur);
        this.elInput.addEventListener("keypress", this.onKeyPress);
        this.elInput.addEventListener("focus", this.showInput.bind(this));
        
        this.elDiv.appendChild(this.elLeftArrow);
        this.elDiv.appendChild(this.elText);
        this.elDiv.appendChild(this.elInput);
        this.elDiv.appendChild(this.elMultiDiff);
        this.elDiv.appendChild(this.elRightArrow);
    }
};
