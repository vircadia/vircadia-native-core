//  listView.js
//
//  Created by David Back on 27 Aug 2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

const SCROLL_ROWS = 2; // number of rows used as scrolling buffer, each time we pass this number of rows we scroll
const FIRST_ROW_INDEX = 3; // the first elRow element's index in the child nodes of the table body

debugPrint = function (message) {
    console.log(message);
};

function ListView(elTableBody, elTableScroll, elTableHeaderRow, createRowFunction, 
                  updateRowFunction, clearRowFunction, WINDOW_NONVARIABLE_HEIGHT) {   
    this.elTableBody = elTableBody;
    this.elTableScroll = elTableScroll;
    this.elTableHeaderRow = elTableHeaderRow;
    
    this.elTopBuffer = null;
    this.elBottomBuffer = null;
    
    this.createRowFunction = createRowFunction;
    this.updateRowFunction = updateRowFunction;
    this.clearRowFunction = clearRowFunction;
    
    // the list of row elements created in the table up to max viewable height plus SCROLL_ROWS rows for scrolling buffer
    this.elRows = [];
    // the list of all row item data to show in the scrolling table, passed to updateRowFunction to set to each row
    this.itemData = [];
    // the current index within the itemData list that is set to the top most elRow element
    this.rowOffset = 0;
    // height of the elRow elements
    this.rowHeight = 0;
    // the previous elTableScroll.scrollTop value when the elRows were last shifted for scrolling
    this.lastRowShiftScrollTop = 0;
    
    this.initialize();
}
    
ListView.prototype = {
    getNumRows: function() {
        return this.elRows.length;
    },
    
    getScrollHeight: function() {
        return this.rowHeight * SCROLL_ROWS;
    },
    
    getFirstVisibleRowIndex: function() {
        return this.rowOffset;
    },
    
    getLastVisibleRowIndex: function() {
        return this.getFirstVisibleRowIndex() + entityList.getNumRows() - 1;
    },
    
    resetToTop: function() {
        this.rowOffset = 0;
        this.lastRowShiftScrollTop = 0;
        this.refreshBuffers();
        this.elTableScroll.scrollTop = 0;
    },

    clear: function() {
        for (let i = 0; i < this.getNumRows(); i++) {
            let elRow = this.elTableBody.childNodes[i + FIRST_ROW_INDEX];
            this.clearRowFunction(elRow);
            elRow.style.display = "none"; // hide cleared rows
        }
    },

    onScroll: function()  {
        var that = this.listView;
        that.scroll();
    },

    scroll: function() {
        let scrollTop = this.elTableScroll.scrollTop;       
        let scrollHeight = this.getScrollHeight();
        let nextRowChangeScrollTop = this.lastRowShiftScrollTop + scrollHeight;
        let totalItems = this.itemData.length;
        let numRows = this.getNumRows();

        // if the top of the scroll area has past the amount of scroll row space since the last point of scrolling and there
        // are still more rows to scroll to then trigger a scroll down by the min of the scroll row space or number of
        // remaining rows below
        // if the top of the scroll area has gone back above the last point of scrolling then trigger a scroll up by min of
        // the scroll row space or number of rows above
        if (scrollTop >= nextRowChangeScrollTop && numRows + this.rowOffset < totalItems) {
            let numScrolls = Math.ceil((scrollTop - nextRowChangeScrollTop) / scrollHeight);
            let numScrollRows = numScrolls * SCROLL_ROWS;
            if (numScrollRows + this.rowOffset + numRows > totalItems) {
                numScrollRows = totalItems - this.rowOffset - numRows;
            }
            this.scrollRows(numScrollRows);
        } else if (scrollTop < this.lastRowShiftScrollTop) {
            let numScrolls = Math.ceil((this.lastRowShiftScrollTop - scrollTop) / scrollHeight);
            let numScrollRows = numScrolls * SCROLL_ROWS;
            if (this.rowOffset - numScrollRows < 0) {
                numScrollRows = this.rowOffset;
            }
            this.scrollRows(-numScrollRows);
        }
    },
    
    scrollRows: function(numScrollRows) {
        let numScrollRowsAbsolute = Math.abs(numScrollRows);
        if (numScrollRowsAbsolute === 0) {
            return;
        }
        
        let scrollDown = numScrollRows > 0; 
        
        let prevTopHeight = parseInt(this.elTopBuffer.getAttribute("height"));
        let prevBottomHeight =  parseInt(this.elBottomBuffer.getAttribute("height"));
        
        // if the number of rows to scroll at once is greater than the total visible number of row elements,
        // then just advance the rowOffset accordingly and allow the refresh below to update all rows
        if (numScrollRowsAbsolute > this.getNumRows()) {
            this.rowOffset += numScrollRows;
        } else {
            // for each row to scroll down, move the top row element to the bottom of the
            // table before the bottom buffer and reset it's row data to the new item
            // for each row to scroll up, move the bottom row element to the top of
            // the table before the top row and reset it's row data to the new item
            for (let i = 0; i < numScrollRowsAbsolute; i++) {
                let topRow = this.elTableBody.childNodes[FIRST_ROW_INDEX];
                let rowToMove = scrollDown ? topRow : this.elTableBody.childNodes[FIRST_ROW_INDEX + this.getNumRows() - 1];
                let rowIndex = scrollDown ? this.getNumRows() + this.rowOffset : this.rowOffset - 1;
                let moveRowBefore = scrollDown ? this.elBottomBuffer : topRow;
                this.elTableBody.removeChild(rowToMove);
                this.elTableBody.insertBefore(rowToMove, moveRowBefore);
                this.updateRowFunction(rowToMove, this.itemData[rowIndex]);
                this.rowOffset += scrollDown ? 1 : -1;
            }
        }
        
        // add/remove the row space that was scrolled away to the top buffer height and last scroll point
        // add/remove the row space that was scrolled away to the bottom buffer height      
        let scrolledSpace = this.rowHeight * numScrollRows;
        let newTopHeight = prevTopHeight + scrolledSpace;
        let newBottomHeight = prevBottomHeight - scrolledSpace;
        this.elTopBuffer.setAttribute("height", newTopHeight);
        this.elBottomBuffer.setAttribute("height", newBottomHeight);
        this.lastRowShiftScrollTop += scrolledSpace;
        
        // if scrolling more than the total number of visible rows at once then refresh all row data
        if (numScrollRowsAbsolute > this.getNumRows()) {
            this.refresh();
        }
    },

    /**
     * Scrolls firstRowIndex with least effort, also tries to make the window include the other selections in case lastRowIndex is set.
     * In the case that firstRowIndex and lastRowIndex are already within the visible bounds then nothing will happen.
     * @param {number} firstRowIndex - The row that will be scrolled to.
     * @param {number} lastRowIndex - The last index of the bound.
     */
    scrollToRow: function (firstRowIndex, lastRowIndex) {
        lastRowIndex = lastRowIndex ? lastRowIndex : firstRowIndex;
        let boundingTop = firstRowIndex * this.rowHeight;
        let boundingBottom = (lastRowIndex * this.rowHeight) + this.rowHeight;
        if ((boundingBottom - boundingTop) > this.elTableScroll.clientHeight) {
            boundingBottom = boundingTop + this.elTableScroll.clientHeight;
        }

        let currentVisibleAreaTop = this.elTableScroll.scrollTop;
        let currentVisibleAreaBottom = currentVisibleAreaTop + this.elTableScroll.clientHeight;

        if (boundingTop < currentVisibleAreaTop) {
            this.elTableScroll.scrollTop = boundingTop;
        } else if (boundingBottom > currentVisibleAreaBottom) {
            this.elTableScroll.scrollTop = boundingBottom - (this.elTableScroll.clientHeight);
        }
    },
    
    refresh: function() {
        // block refreshing before rows are initialized
        let numRows = this.getNumRows();
        if (numRows === 0) {
            return;
        }
        
        let prevScrollTop = this.elTableScroll.scrollTop;
        
        // start with all row data cleared and initially set to invisible
        this.clear();
        
        // if we are at the bottom of the list adjust row offset to make sure all rows stay in view
        this.refreshRowOffset();
        
        // update all row data and set rows visible until max visible items reached
        for (let i = 0; i < numRows; i++) {
            let rowIndex = i + this.rowOffset;
            if (rowIndex >= this.itemData.length) {
                break;
            }
            let rowElementIndex = i + FIRST_ROW_INDEX;
            let elRow = this.elTableBody.childNodes[rowElementIndex];
            let itemData = this.itemData[rowIndex];
            this.updateRowFunction(elRow, itemData);
            elRow.style.display = ""; // make sure the row is visible
        }
        
        // update the top and bottom buffer heights to adjust for above changes
        this.refreshBuffers();

        // adjust the last row shift scroll point based on how much the current scroll point changed
        let scrollTopDifference =  this.elTableScroll.scrollTop - prevScrollTop;
        if (scrollTopDifference !== 0) {
            this.lastRowShiftScrollTop += scrollTopDifference;
            if (this.lastRowShiftScrollTop < 0) {
                this.lastRowShiftScrollTop = 0;
            }
        }
    },
    
    refreshBuffers: function() {
        // top buffer height is the number of hidden rows above the top row
        let topHiddenRows = this.rowOffset;
        let topBufferHeight = this.rowHeight * topHiddenRows;
        this.elTopBuffer.setAttribute("height", topBufferHeight);
        
        // bottom buffer height is the number of hidden rows below the bottom row (last scroll buffer row)
        let bottomHiddenRows = this.itemData.length - this.getNumRows() - this.rowOffset;
        let bottomBufferHeight = this.rowHeight * bottomHiddenRows;
        if (bottomHiddenRows < 0) {
            bottomBufferHeight = 0;
        }
        this.elBottomBuffer.setAttribute("height", bottomBufferHeight);
    },
    
    refreshRowOffset: function() {
        // make sure the row offset isn't causing visible rows to pass the end of the item list and is clamped to 0
        var numRows = this.getNumRows();
        if (this.rowOffset + numRows > this.itemData.length) {
            this.rowOffset = this.itemData.length - numRows;
        }
        if (this.rowOffset < 0) {
            this.rowOffset = 0;
        }
    },

    onResize: function() {
        var that = this.listView;
        that.resize();
    },
    
    resize: function() {        
        if (!this.elTableBody || !this.elTableScroll) {
            debugPrint("ListView.resize - no valid table body or table scroll element");
            return;
        }

        let prevScrollTop = this.elTableScroll.scrollTop;         

        // take up available window space
        this.elTableScroll.style.height = window.innerHeight - WINDOW_NONVARIABLE_HEIGHT;
        let viewableHeight = parseInt(this.elTableScroll.style.height) + 1;

        // remove all existing row elements and clear row list
        for (let i = 0; i < this.getNumRows(); i++) {
            let elRow = this.elRows[i];
            this.elTableBody.removeChild(elRow);
        }
        this.elRows = [];
        
        // create new row elements inserted between the top and bottom buffers up until the max viewable scroll area
        let usedHeight = 0;
        while (usedHeight < viewableHeight) {
            let newRow = this.createRowFunction();
            this.elTableBody.insertBefore(newRow, this.elBottomBuffer);
            this.rowHeight = newRow.offsetHeight;
            usedHeight += this.rowHeight;     
            this.elRows.push(newRow);
        }
        
        // add SCROLL_ROWS extras rows for scrolling buffer purposes
        for (let i = 0; i < SCROLL_ROWS; i++) {
            let scrollRow = this.createRowFunction();
            this.elTableBody.insertBefore(scrollRow, this.elBottomBuffer);
            this.elRows.push(scrollRow);
        }

        let ths = this.elTableHeaderRow;
        let tds = this.getNumRows() > 0 ? this.elRows[0].childNodes : [];
        if (!ths) {
            debugPrint("ListView.resize - no valid table header row");
        } else if (tds.length !== ths.length) {
            debugPrint("ListView.resize - td list size " + tds.length + " does not match th list size " + ths.length);
        }
        // update the widths of the header cells to match the body cells (using first body row)
        for (let i = 0; i < ths.length; i++) {
            ths[i].width = tds[i].offsetWidth;
        }
        
        // restore the scroll point to the same scroll point from before above changes
        this.elTableScroll.scrollTop = prevScrollTop;
        
        this.refresh();
    },
    
    initialize: function() {
        if (!this.elTableBody || !this.elTableScroll) {
            debugPrint("ListView.initialize - no valid table body or table scroll element");
            return;
        }
        
        // delete initial blank row
        this.elTableBody.deleteRow(0);
        
        this.elTopBuffer = document.createElement("tr");
        this.elTableBody.appendChild(this.elTopBuffer);
        this.elTopBuffer.setAttribute("height", 0);
        
        this.elBottomBuffer = document.createElement("tr");
        this.elTableBody.appendChild(this.elBottomBuffer);
        this.elBottomBuffer.setAttribute("height", 0);

        this.elTableScroll.listView = this;
        this.elTableScroll.onscroll = this.onScroll;
        window.listView = this;
        window.onresize = this.onResize;

        // initialize all row elements
        this.resize();
    }
};
