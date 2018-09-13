//  listView.js
//
//  Created by David Back on 27 Aug 2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

debugPrint = function (message) {
    console.log(message);
};

ListView = function(tableId, tableBodyId, tableScrollId, createRowFunction, updateRowFunction, clearRowFunction) {
    var that = {};
    
    var SCROLL_ROWS = 2;
    var FIRST_ROW_INDEX = 3;
    
    var elTable = document.getElementById(tableId);
    var elTableBody = document.getElementById(tableBodyId);
    var elTableScroll = document.getElementById(tableScrollId);
    
    var elTopBuffer = null;
    var elBottomBuffer = null;

    var visibleItemData = [];
    var elRows = [];
    
    var rowOffset = 0;
    var rowHeight = 0;
    var lastRowChangeScrollTop = 0;
    
    that.getNumRows = function() {
        return elRows.length;
    };
    
    that.getScrollHeight = function() {
        return rowHeight * SCROLL_ROWS;
    };
    
    that.getRowOffset = function() {
        return rowOffset;
    };
    
    that.resetRowOffset = function() {
        rowOffset = 0;
        lastRowChangeScrollTop = 0;
    };
    
    that.decrementRowOffset = function() {          
        // increase top buffer height to account for the lost row space
        var topHeight = parseInt(elTopBuffer.getAttribute("height"));       
        var newTopHeight = topHeight + rowHeight;
        elTopBuffer.setAttribute("height", newTopHeight);
        
        rowOffset--;
        that.refresh();
        lastRowChangeScrollTop = topHeight;
        
        return rowOffset;
    };
    
    that.setVisibleItemData = function(itemData) {
        visibleItemData = itemData;
    };
        
    that.clear = function() {
        for (var i = 0; i < that.getNumRows(); i++) {
            var elRow = elTableBody.childNodes[i + FIRST_ROW_INDEX];
            clearRowFunction(elRow);
            elRow.style.display = "none";
        }
    };
    
    that.scrollDown = function(numScrollRows) {     
        var prevTopHeight = parseInt(elTopBuffer.getAttribute("height"));
        var prevBottomHeight =  parseInt(elBottomBuffer.getAttribute("height"));
        
        // for each row to scroll down, move the top row element to the bottom of the
        // table before the bottom buffer and reset it's row data to the new item
        for (var i = 0; i < numScrollRows; i++) {
            var rowMovedTopToBottom = elTableBody.childNodes[FIRST_ROW_INDEX];
            var rowIndex = that.getNumRows() + rowOffset;
            elTableBody.removeChild(rowMovedTopToBottom);
            elTableBody.insertBefore(rowMovedTopToBottom, elBottomBuffer);
            updateRowFunction(rowMovedTopToBottom, visibleItemData[rowIndex]);
            rowOffset++;
        }
        
        // add the row space that was scrolled away to the top buffer height and last scroll point
        // remove the row space that was scrolled away from the bottom buffer height 
        var scrolledSpace = rowHeight * numScrollRows;
        var newTopHeight = prevTopHeight + scrolledSpace;
        var newBottomHeight = prevBottomHeight - scrolledSpace;
        elTopBuffer.setAttribute("height", newTopHeight);
        elBottomBuffer.setAttribute("height", newBottomHeight);
        lastRowChangeScrollTop += scrolledSpace;
    };
    
    that.scrollUp = function(numScrollRows) {
        var prevTopHeight = parseInt(elTopBuffer.getAttribute("height"));
        var prevBottomHeight =  parseInt(elBottomBuffer.getAttribute("height"));

        // for each row to scroll up, move the bottom row element to the top of
        // the table before the top row and reset it's row data to the new item
        for (var i = 0; i < numScrollRows; i++) {
            var topRow = elTableBody.childNodes[FIRST_ROW_INDEX];
            var rowMovedBottomToTop = elTableBody.childNodes[FIRST_ROW_INDEX + that.getNumRows() - 1];
            var rowIndex = rowOffset - 1;
            elTableBody.removeChild(rowMovedBottomToTop);
            elTableBody.insertBefore(rowMovedBottomToTop, topRow);
            updateRowFunction(rowMovedBottomToTop, visibleItemData[rowIndex]);
            rowOffset--;
        }
        
        // remove the row space that was scrolled away from the top buffer height and last scroll point
        // add the row space that was scrolled away to the bottom buffer height
        var scrolledSpace = rowHeight * numScrollRows;
        var newTopHeight = prevTopHeight - scrolledSpace;
        var newBottomHeight = prevBottomHeight + scrolledSpace;
        elTopBuffer.setAttribute("height", newTopHeight);
        elBottomBuffer.setAttribute("height", newBottomHeight);
        lastRowChangeScrollTop -= scrolledSpace;
    };
    
    that.onScroll = function()  {
        var scrollTop = elTableScroll.scrollTop;
        var scrollHeight = that.getScrollHeight();
        var nextRowChangeScrollTop = lastRowChangeScrollTop + scrollHeight;
        var totalItems = visibleItemData.length;
        var numRows = that.getNumRows();
        
        // if the top of the scroll area has past the amount of scroll row space since the last point of scrolling and there
        // are still more rows to scroll to then trigger a scroll down by the min of the scroll row space or number of
        // remaining rows below
        // if the top of the scroll area has gone back above the last point of scrolling then trigger a scroll up by min of
        // the scroll row space or number of rows above
        if (scrollTop >= nextRowChangeScrollTop && numRows + rowOffset < totalItems) {
            var numScrolls = Math.ceil((scrollTop - nextRowChangeScrollTop) / scrollHeight);
            var numScrollRows = numScrolls * SCROLL_ROWS;
            if (numScrollRows + rowOffset + numRows > totalItems) {
                numScrollRows = totalItems - rowOffset - numRows;
            }
            that.scrollDown(numScrollRows);
        } else if (scrollTop < lastRowChangeScrollTop) {
            var numScrolls = Math.ceil((lastRowChangeScrollTop - scrollTop) / scrollHeight);
            var numScrollRows = numScrolls * SCROLL_ROWS;
            if (rowOffset - numScrollRows < 0) {
                numScrollRows = rowOffset;
            }
            that.scrollUp(numScrollRows);
        }
    };
    
    that.refresh = function() {
        // block refreshing before rows are initialized
        var numRows = that.getNumRows();
        if (numRows === 0) {
            return;
        }
        
        // start with all row data cleared and initially set to invisible
        that.clear();
        
        // update all row data and set rows visible until max visible items reached
        for (var i = 0; i < numRows; i++) {
            var rowIndex = i + rowOffset;
            if (rowIndex >= visibleItemData.length) {
                break;
            }
            var rowElementIndex = i + FIRST_ROW_INDEX;
            var elRow = elTableBody.childNodes[rowElementIndex];
            var itemData = visibleItemData[rowIndex];
            updateRowFunction(elRow, itemData);
            elRow.style.display = "";
        }
        
        // top buffer height is the number of hidden rows above the top row
        var topHiddenRows = rowOffset;
        var topBufferHeight = rowHeight * topHiddenRows;
        elTopBuffer.setAttribute("height", topBufferHeight);
        
        // bottom buffer height is the number of hidden rows below the bottom row (last scroll buffer row)
        var bottomHiddenRows = visibleItemData.length - numRows - rowOffset;
        var bottomBufferHeight = rowHeight * bottomHiddenRows;
        if (bottomHiddenRows < 0) {
            bottomBufferHeight = 0;
        }
        elBottomBuffer.setAttribute("height", bottomBufferHeight);
    };
    
    that.resetRows = function(viewableHeight) {
        if (!elTableBody) {
            debugPrint("ListView.resetRows - no valid table body element");
            return;
        }
        
        that.resetRowOffset();
        
        elTopBuffer.setAttribute("height", 0);
        elBottomBuffer.setAttribute("height", 0);

        // clear out any existing rows
        for (var i = 0; i < that.getNumRows(); i++) {
            var elRow = elRows[i];
            elTableBody.removeChild(elRow);
        }
        elRows = [];
        
        // create new row elements inserted between the top and bottom buffers that can fit into viewable scroll area
        var usedHeight = 0;
        while(usedHeight < viewableHeight) {
            var newRow = createRowFunction();
            elTableBody.insertBefore(newRow, elBottomBuffer);
            rowHeight = elTableBody.offsetHeight - usedHeight;
            usedHeight = elTableBody.offsetHeight;     
            elRows.push(newRow);
        }
        
        // add extras rows for scrolling buffer purposes
        for (var i = 0; i < SCROLL_ROWS; i++) {
            var scrollRow = createRowFunction();
            elTableBody.insertBefore(scrollRow, elBottomBuffer);
            elRows.push(scrollRow);
        }
    };
    
    that.initialize = function() {
        if (!elTableBody || !elTableScroll) {
            debugPrint("ListView.initialize - no valid table body or table scroll element");
            return;
        }
        
        // delete initial blank row
        elTableBody.deleteRow(0);
        
        elTopBuffer = document.createElement("tr");
        elTableBody.appendChild(elTopBuffer);
        elTopBuffer.setAttribute("height", 0);
        
        elBottomBuffer = document.createElement("tr");
        elTableBody.appendChild(elBottomBuffer);
        elBottomBuffer.setAttribute("height", 0);
        
        elTableScroll.onscroll = that.onScroll;
    };
    
    that.initialize();
    
    return that;
}
