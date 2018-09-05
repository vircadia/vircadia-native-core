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
    
    var itemData = {};
    
    var rowOffset = 0;
    var numRows = 0;
    var rowHeight = 0;
    var lastRowChangeScrollTop = 0;
    
    that.addItem = function(id, data) {
        if (itemData[id] === undefined) {
            itemData[id] = data;
        }
    };
    
    that.updateItem = function(id, data) {
        if (itemData[id] !== undefined) {

        }
    };
    
    that.removeItem = function(id) {
        if (itemData[id] !== undefined) {
            delete itemData[id];
        }
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
    
    that.scrollDown = function() {
        var prevTopHeight = parseInt(elTopBuffer.getAttribute("height"));
        var prevBottomHeight =  parseInt(elBottomBuffer.getAttribute("height"));
        
        for (var i = 0; i < SCROLL_ROWS; i++) {
            var topRow = elTableBody.childNodes[FIRST_ROW_INDEX];
            elTableBody.removeChild(topRow);
            elTableBody.insertBefore(topRow, elBottomBuffer);
            var rowIndex = 0;
            for (var id in itemData) {
                if (rowIndex === numRows + rowOffset) {
                    topRow.setAttribute("id", id);
                    updateRowFunction(topRow, itemData[id]);
                    break;
                }
                rowIndex++;
            }
            rowOffset++;
        }
        
        var newTopHeight = prevTopHeight + (rowHeight * SCROLL_ROWS);
        var newBottomHeight = prevBottomHeight - (rowHeight * SCROLL_ROWS);
        elTopBuffer.setAttribute("height", newTopHeight);
        elBottomBuffer.setAttribute("height", newBottomHeight);
        lastRowChangeScrollTop += rowHeight * SCROLL_ROWS;
    };
    
    that.scrollUp = function() {
        var prevTopHeight = parseInt(elTopBuffer.getAttribute("height"));
        var prevBottomHeight =  parseInt(elBottomBuffer.getAttribute("height"));
        
        for (var i = 0; i < SCROLL_ROWS; i++) {
            var topRow = elTableBody.childNodes[FIRST_ROW_INDEX];
            var bottomRow = elTableBody.childNodes[FIRST_ROW_INDEX + numRows - 1];
            elTableBody.removeChild(bottomRow);
            elTableBody.insertBefore(bottomRow, topRow);
            var rowIndex = 0;
            for (var id in itemData) {
                if (rowIndex === rowOffset - 1) {
                    bottomRow.setAttribute("id", id);
                    updateRowFunction(bottomRow, itemData[id]);
                    break;
                }
                rowIndex++;
            }
            rowOffset--;
        }
        
        var newTopHeight = prevTopHeight - (rowHeight * SCROLL_ROWS);
        var newBottomHeight = prevBottomHeight + (rowHeight * SCROLL_ROWS);
        elTopBuffer.setAttribute("height", newTopHeight);
        elBottomBuffer.setAttribute("height", newBottomHeight);
        lastRowChangeScrollTop -= rowHeight * SCROLL_ROWS;
    };
    
    that.onScroll = function()  {
        var scrollTop = elTableScroll.scrollTop;
        var nextRowChangeScrollTop = lastRowChangeScrollTop + (rowHeight * SCROLL_ROWS);
        var totalItems = Object.keys(itemData).length;
        
        if (scrollTop >= nextRowChangeScrollTop && numRows + rowOffset < totalItems) {  
            that.scrollDown();
        } else if (scrollTop < lastRowChangeScrollTop && rowOffset >= SCROLL_ROWS) {
            that.scrollUp();
        }
    };
    
    that.refresh = function() {
        var rowIndex = 0;
        for (var id in itemData) {
            if (rowIndex >= rowOffset) {
                var rowElementIndex = rowIndex + FIRST_ROW_INDEX;
                var elRow = elTableBody.childNodes[rowElementIndex];
                var data = itemData[id];
                elRow.setAttribute("id", id);
                updateRowFunction(elRow, data); 
            }           
            
            rowIndex++;
            
            if (rowIndex - rowOffset === numRows) {
                break;
            }
        }
        
        var totalItems = Object.keys(itemData).length;
        var bottomHiddenRows = totalItems - numRows - rowOffset;
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