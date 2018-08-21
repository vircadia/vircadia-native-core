//
//  PSFListModel.qml
//  qml/hifi/commerce/common
//
//  PSFListModel
// "PSF" stands for:
//     - Paged
//     - Sortable
//     - Filterable
//
//  Created by Zach Fox on 2018-05-15
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7

ListModel {
    id: root;
    // Used when printing debug statements
    property string listModelName: endpoint;
    
    // Parameters. Even if you override getPage, below, please set these for clarity and consistency, when applicable.
    // E.g., your getPage function could refer to this sortKey, etc.
    property string endpoint;
    property string sortProperty;  // Currently only handles sorting on one column, which fits with current needs and tables.
    property bool sortAscending;
    property string sortKey: !sortProperty ? '' : (sortProperty + "," + (sortAscending ? "ASC" : "DESC"));
    property string searchFilter: "";
    property string tagsFilter;

    // QML fires the following changed handlers even when first instantiating the Item. So we need a guard against firing them too early.
    property bool initialized: false;
    onEndpointChanged: if (initialized) { getFirstPage('delayClear'); }
    onSortKeyChanged:  if (initialized) { getFirstPage('delayClear'); }
    onSearchFilterChanged: if (initialized) { getFirstPage('delayClear'); }
    onTagsFilterChanged: if (initialized) { getFirstPage('delayClear'); }

    // When considering a value for `itemsPerPage` in YOUR model, consider the following:
    //     - If your ListView delegates are of variable width/height, ensure you select
    //     an `itemsPerPage` value that would be sufficient to show one full page of data
    //     if all of the delegates were at their minimum heights.
    //     - If your first ListView delegate contains some special data (as in WalletHome's
    //     "Recent Activity" view), beware that your `itemsPerPage` value may _never_ reasonably be
    //     high enough such that the first page of data causes the view to be one-screen in height
    //     after retrieving the first page. This means data will automatically pop-in (after a short delay)
    //     until the combined heights of your View's delegates reach one-screen in height OR there is
    //     no more data to retrieve. See "needsMoreVerticalResults()" below.
    property int itemsPerPage: 100;

    // State.
    property int currentPageToRetrieve: 0;  // 0 = before first page. -1 = we have them all. Otherwise 1-based page number.
    property bool retrievedAtLeastOnePage: false;
    // We normally clear on reset. But if we want to "refresh", we can delay clearing the model until we get a result.
    // Not normally set directly, but rather by giving a truthy argument to getFirstPage(true);
    property bool delayedClear: false;
    function resetModel() {
        currentPageToRetrieve = 1;
        retrievedAtLeastOnePage = false;
        if (!delayedClear) { root.clear(); }
        totalPages = 0;
        totalEntries = 0;
    }

    // Page processing.

    // Override to return one property of data, and/or to transform the elements. Must return an array of model elements.
    property var processPage: function (data) { return data; }

    property var listView; // Optional. For debugging, or for having the scroll handler automatically call getNextPage.
    property var flickable: listView && (listView.flickableItem || listView);
    // 2: get two pages before you need it (i.e. one full page before you reach the end).
    // 1: equivalent to paging when reaching end (and not before).
    // 0: don't getNextPage on scroll at all here. The application code will do it.
    property real pageAhead: 2.0;
    function needsEarlyYFetch() {
         return flickable
            && !flickable.atYBeginning
            && (flickable.contentY - flickable.originY) >= (flickable.contentHeight - (pageAhead * flickable.height));
    }
    function needsEarlyXFetch() {
        return flickable
            && !flickable.atXBeginning
            && (flickable.contentX - flickable.originX) >= (flickable.contentWidth - (pageAhead * flickable.width));
    }
    function getNextPageIfHorizontalScroll() {
        if (needsEarlyXFetch()) { getNextPage(); }
    }
    function getNextPageIfVerticalScroll() {
        if (needsEarlyYFetch()) { getNextPage(); }
    }
    function needsMoreHorizontalResults() {
        return flickable
            && currentPageToRetrieve > 0
            && retrievedAtLeastOnePage
            && flickable.contentWidth < flickable.width;
    }
    function needsMoreVerticalResults() {
        return flickable
            && currentPageToRetrieve > 0
            && retrievedAtLeastOnePage
            && flickable.contentHeight < flickable.height;
    }
    function getNextPageIfNotEnoughHorizontalResults() {
        if (needsMoreHorizontalResults()) {
            getNextPage();
        }
    }
    function getNextPageIfNotEnoughVerticalResults() {
        if (needsMoreVerticalResults()) {
            getNextPage();
        }
    }

    Component.onCompleted: {
        initialized = true;
        if (flickable && pageAhead > 0.0) {
            // Pun: Scrollers are usually one direction or another, such that only one of the following will actually fire.
            flickable.contentXChanged.connect(getNextPageIfHorizontalScroll);
            flickable.contentYChanged.connect(getNextPageIfVerticalScroll);
            flickable.contentWidthChanged.connect(getNextPageIfNotEnoughHorizontalResults);
            flickable.contentHeightChanged.connect(getNextPageIfNotEnoughVerticalResults);
        }
    }

    property int totalPages: 0;
    property int totalEntries: 0;
    // Check consistency and call processPage.
    function handlePage(error, response, cb) {
        var processed;
        console.debug('handlePage', listModelName, additionalFirstPageRequested, error, JSON.stringify(response));
        function fail(message) {
            console.warn("Warning page fail", listModelName, JSON.stringify(message));
            currentPageToRetrieve = -1;
            requestPending = false;
            delayedClear = false;
        }
        if (error || (response.status !== 'success')) {
            return fail(error || response.status);
        }
        if (!requestPending) {
            return fail("No request in flight.");
        }
        requestPending = false;
        if (response.current_page && response.current_page !== currentPageToRetrieve) { // Not all endpoints specify this property.
            return fail("Mismatched page, expected:" + currentPageToRetrieve);
        }
        totalPages = response.total_pages || 0;
        totalEntries = response.total_entries || 0;
        processed = processPage(response.data || response);
        if (totalPages && (totalPages === currentPageToRetrieve)) {
            currentPageToRetrieve = -1;
        }

        if (delayedClear) {
            root.clear();
            delayedClear = false;
        }
        root.append(processed); // FIXME keep index steady, and apply any post sort
        retrievedAtLeastOnePage = true;
        // Suppose two properties change at once, and both of their change handlers request a new first page.
        // (An example is when the a filter box gets cleared with text in it, so that the search and tags are both reset.)
        // Or suppose someone just types new search text quicker than the server response.
        // In these cases, we would have multiple requests in flight, and signal based responses aren't generally very good
        // at matching up the right handler with the right message. Rather than require all the APIs to carefully handle such,
        // and also to cut down on useless requests, we take care of that case here.
        if (additionalFirstPageRequested) {
            console.debug('deferred getFirstPage', listModelName);
            additionalFirstPageRequested = false;
            getFirstPage('delayedClear', cb);
        } else if (cb) {
            cb();
        }
    }
    function debugView(label) {
        if (!listView) { return; }
        console.debug(label, listModelName, 'perPage:', itemsPerPage, 'count:', listView.count,
            'index:', listView.currentIndex, 'section:', listView.currentSection,
            'atYBeginning:', listView.atYBeginning, 'atYEnd:', listView.atYEnd,
            'y:', listView.y, 'contentY:', listView.contentY);
    }

    // Override either http or getPage.
    property var http; // An Item that has a request function.
    property var getPage: function (cb) {  // Any override MUST call handlePage(), above, even if results empty.
        if (!http) { return console.warn("Neither http nor getPage was set for", listModelName); }
        // If it is a path starting with slash, add the metaverseServer domain.
        var url = /^\//.test(endpoint) ? (Account.metaverseServerURL + endpoint) : endpoint;
        var parameters = [
            'per_page=' + itemsPerPage,
            'page=' + currentPageToRetrieve
        ];
        if (searchFilter) {
            parameters.splice(parameters.length, 0, 'search=' + searchFilter);
        }
        if (sortKey) {
            parameters.splice(parameters.length, 0, 'sort=' + sortKey);
        }

        var parametersSeparator = /\?/.test(url) ? '&' : '?';
        url = url + parametersSeparator + parameters.join('&');
        console.debug('getPage', listModelName, currentPageToRetrieve);
        http.request({uri: url}, cb ? function (error, result) { handlePage(error, result, cb); } : handlePage);
    }

    // Start the show by retrieving data according to `getPage()`.
    // It can be custom-defined by this item's Parent.
    property var getFirstPage: function (delayClear, cb) {
        if (requestPending) {
            console.debug('deferring getFirstPage', listModelName);
            additionalFirstPageRequested = true;
            return;
        }
        delayedClear = !!delayClear;
        resetModel();
        requestPending = true;
        console.debug("getFirstPage", listModelName, currentPageToRetrieve);
        getPage(cb);
    }
    property bool additionalFirstPageRequested: false;
    property bool requestPending: false; // For de-bouncing getNextPage.
    // This function, will get the _next_ page of data according to `getPage()`.
    // It can be custom-defined by this item's Parent. Typical usage:
    // ListView {
    //    id: theList
    //    model: thisPSFListModelId
    //    onAtYEndChanged: if (theList.atYEnd && !theList.atYBeginning) { thisPSFListModelId.getNextPage(); }
    //    ...}
    property var getNextPage: function () {
        if (requestPending || currentPageToRetrieve < 0) {
            return;
        }
        currentPageToRetrieve++;
        console.debug("getNextPage", listModelName, currentPageToRetrieve);
        requestPending = true;
        getPage();
    }
}