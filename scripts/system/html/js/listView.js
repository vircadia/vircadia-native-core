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
    
    var rowOffset = 0;
    var numRows = 0;
    var rowHeight = 0;
    var lastRowChangeScrollTop = 0;
    
    that.setVisibleItemData = function(itemData) {
        visibleItemData = itemData;
    };
    
    that.clear = function() {
        for (var i = 0; i < numRows; i++) {
            var elRow = elTableBody.childNodes[i + FIRST_ROW_INDEX];
            clearRowFunction(elRow);
        }
    };
    
    that.setSortKey = function(key) {
        
    };
    
    that.setFilter = function(filter) {
        
    };
    
    that.scrollDown = function(numScrollRows) {     
        var prevTopHeight = parseInt(elTopBuffer.getAttribute("height"));
        var prevBottomHeight =  parseInt(elBottomBuffer.getAttribute("height"));
        
        for (var i = 0; i < numScrollRows; i++) {
            var rowMovedTopToBottom = elTableBody.childNodes[FIRST_ROW_INDEX];
            var rowIndex = numRows + rowOffset;
            elTableBody.removeChild(rowMovedTopToBottom);
            elTableBody.insertBefore(rowMovedTopToBottom, elBottomBuffer);
            updateRowFunction(rowMovedTopToBottom, visibleItemData[rowIndex]);
            rowOffset++;
        }
        
        var newTopHeight = prevTopHeight + (rowHeight * numScrollRows);
        var newBottomHeight = prevBottomHeight - (rowHeight * numScrollRows);
        elTopBuffer.setAttribute("height", newTopHeight);
        elBottomBuffer.setAttribute("height", newBottomHeight);
        lastRowChangeScrollTop += rowHeight * numScrollRows;
    };
    
    that.scrollUp = function(numScrollRows) {
        var prevTopHeight = parseInt(elTopBuffer.getAttribute("height"));
        var prevBottomHeight =  parseInt(elBottomBuffer.getAttribute("height"));

        for (var i = 0; i < numScrollRows; i++) {
            var topRow = elTableBody.childNodes[FIRST_ROW_INDEX];
            var rowMovedBottomToTop = elTableBody.childNodes[FIRST_ROW_INDEX + numRows - 1];
            var rowIndex = rowOffset - 1;
            elTableBody.removeChild(rowMovedBottomToTop);
            elTableBody.insertBefore(rowMovedBottomToTop, topRow);
            updateRowFunction(rowMovedBottomToTop, visibleItemData[rowIndex]);
            rowOffset--;
        }
        
        var newTopHeight = prevTopHeight - (rowHeight * numScrollRows);
        var newBottomHeight = prevBottomHeight + (rowHeight * numScrollRows);
        elTopBuffer.setAttribute("height", newTopHeight);
        elBottomBuffer.setAttribute("height", newBottomHeight);
        lastRowChangeScrollTop -= rowHeight * numScrollRows;
    };
    
    that.onScroll = function()  {
        var scrollTop = elTableScroll.scrollTop;
        var nextRowChangeScrollTop = lastRowChangeScrollTop + (rowHeight * SCROLL_ROWS);
        var scrollHeight = rowHeight * SCROLL_ROWS;
        var totalItems = visibleItemData.length;
        
        if (scrollTop >= nextRowChangeScrollTop && numRows + rowOffset < totalItems) {
            var numScrolls = Math.ceil((scrollTop - nextRowChangeScrollTop) / scrollHeight);
            var numScrollRows = numScrolls * SCROLL_ROWS;
            if (numScrollRows + rowOffset + numRows > totalItems) {
                numScrollRows = totalItems - rowOffset - numRows;
            }
            that.scrollDown(numScrollRows);
        } else if (scrollTop < lastRowChangeScrollTop && rowOffset >= SCROLL_ROWS) {
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
        if (numRows === 0) {
            return;
        }
        
        for (var i = 0; i < numRows; i++) {
            var rowIndex = i + rowOffset;
            if (rowIndex >= visibleItemData.length) {
                break;
            }
            var rowElementIndex = i + FIRST_ROW_INDEX;
            var elRow = elTableBody.childNodes[rowElementIndex];
            var itemData = visibleItemData[rowIndex];
            updateRowFunction(elRow, itemData);          
        }
        
        var bottomHiddenRows = visibleItemData.length - numRows - rowOffset;
        var bottomBufferHeight = rowHeight * bottomHiddenRows;
        if (bottomHiddenRows < 0) {
            bottomBufferHeight = 0;
        }
        elBottomBuffer.setAttribute("height", bottomBufferHeight);
    };
    
    that.initialize = function(viewableHeight) {
        if (!elTable || !elTableBody) {
            debugPrint("ListView - no valid table/body element");
            return;
        }
        
        elTableScroll.onscroll = that.onScroll;

        // clear out any existing rows
        while(elTableBody.rows.length > 0) {
            elTableBody.deleteRow(0);
        }
                
        elTopBuffer = document.createElement("tr");
        elTopBuffer.setAttribute("height", 0);
        elTableBody.appendChild(elTopBuffer);
        
        elBottomBuffer = document.createElement("tr");
        elBottomBuffer.setAttribute("height", 0);
        elTableBody.appendChild(elBottomBuffer);
        
        var maxHeight = viewableHeight;
        var usedHeight = 0;
        while(usedHeight < maxHeight) {
            var newRow = createRowFunction(elBottomBuffer);
            rowHeight = elTableBody.offsetHeight - usedHeight;
            usedHeight = elTableBody.offsetHeight;          
            numRows++;
        }
        
        // extra rows for scrolling purposes
        for (var i = 0; i < SCROLL_ROWS; i++) {
            var scrollRow = createRowFunction(elBottomBuffer);
            numRows++;
        }
    }
    
    return that;
}