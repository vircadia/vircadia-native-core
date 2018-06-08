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

Item {
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
    Component.onCompleted: initialized = true;
    onEndpointChanged: if (initialized) { getFirstPage('delayClear'); }
    onSortKeyChanged:  if (initialized) { getFirstPage('delayClear'); }
    onSearchFilterChanged: {
        if (!initialized) { return; }
        getFirstPage('delayClear');
    }
    onTagsFilterChanged: if (initialized) { getFirstPage('delayClear'); }
    property int itemsPerPage: 100;

    // State.
    property int currentPageToRetrieve: 0;  // 0 = before first page. -1 = we have them all. Otherwise 1-based page number.
    property bool retrievedAtLeastOnePage: false;
    // We normally clear on reset. But if we want to "refresh", we can delay clearing the model until we get a result.
    // Not normally set directly, but rather by giving a truthy argument to getFirstPage(true);
    property bool delayedClear: false;
    function resetModel() {
        if (!delayedClear) { finalModel.clear(); }
        currentPageToRetrieve = 1;
        retrievedAtLeastOnePage = false;
    }

    // Processing one page.

    // Override to return one property of data, and/or to transform the elements. Must return an array of model elements.
    property var processPage: function (data) { return data; }

    property var listView; // Optional. For debugging.
    // Check consistency and call processPage.
    function handlePage(error, response) {
        var processed;
        console.debug('handlePage', listModelName, error, JSON.stringify(response));
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
        processed = processPage(response.data || response);
        if (response.total_pages && (response.total_pages === currentPageToRetrieve)) {
            currentPageToRetrieve = -1;
        }

        if (delayedClear) {
            finalModel.clear();
            delayedClear = false;
        }
        finalModel.append(processed); // FIXME keep index steady, and apply any post sort
        retrievedAtLeastOnePage = true;
        debugView('after handlePage');
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
    property var getPage: function () {  // Any override MUST call handlePage(), above, even if results empty.
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
        http.request({uri: url}, handlePage);
    }

    // Start the show by retrieving data according to `getPage()`.
    // It can be custom-defined by this item's Parent.
    property var getFirstPage: function (delayClear) {
        delayedClear = !!delayClear;
        resetModel();
        requestPending = true;
        console.debug("getFirstPage", listModelName, currentPageToRetrieve);
        getPage();
    }
    
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

    // Redefining members and methods so that the parent of this Item
    // can use PSFListModel as they would a regular ListModel
    property alias model: finalModel;
    property alias count: finalModel.count;
    function clear() { finalModel.clear(); }
    function get(index) { return finalModel.get(index); }
    function remove(index) { return finalModel.remove(index); }
    function setProperty(index, prop, value) { return finalModel.setProperty(index, prop, value); }
    function move(from, to, n) { return finalModel.move(from, to, n); }
    function insert(index, newElement) { finalModel.insert(index, newElement); }
    function append(newElements) { finalModel.append(newElements); }

    ListModel {
        id: finalModel;
    }
}