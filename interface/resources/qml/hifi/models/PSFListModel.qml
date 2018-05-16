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
    property string listModelName: "";
    
    // Holds the value of the page that'll be retrieved the next time `getPage()` is called
    property int currentPageToRetrieve: 1;

    // If defined, the endpoint that `getPage()` will hit (as long as there isn't a custom `getPage()`
    // that does something fancy)
    property string endpoint;
    // If defined, the sort key used when calling the above endpoint.
    // (as long as there isn't a custom `getPage()` that does something fancy)
    property string sortKey;
    // If defined, the search filter used when calling the above endpoint.
    // (as long as there isn't a custom `getPage()` that does something fancy)
    property string searchFilter;
    // If defined, the tags filter used when calling the above endpoint.
    // (as long as there isn't a custom `getPage()` that does something fancy)
    property string tagsFilter;
    // The number of items that'll be retrieved per page when calling `getPage()`
    // (as long as there isn't a custom `getPage()` that does something fancy)
    property int itemsPerPage: 100;

    // This function, by default, will retrieve data from the above-defined `endpoint` with the
    // sort and filter data as set above. It can be custom-defined by this item's Parent.
    property var getPage: function() {
        // Put code here that calls the `endpoint` with the proper `sortKey`, `searchFilter`, and `tagsFilter`.
        // Whatever code goes here should define the below `pageRetrieved()` as
        // the callback for after the page is asynchronously retrieved.

        // The parent of this Item can also define custom `getPage()` and `pageRetrieved()` functions.
        // See `WalletHome.qml` as an example of a file that does this. `WalletHome.qml` must use that method because
        // it hits an endpoint that must be authenticated via the Wallet.
        console.log("default getPage()");
    }
    // This function, by default, will handle the data retrieved using `getPage()` above.
    // It can be custom-defined by this item's Parent.
    property var pageRetrieved: function() {
        console.log("default pageRetrieved()");
    }
    
    // This function, by default, will get the _next_ page of data according to `getPage()` if there
    // isn't a pending request and if there's more data to retrieve.
    // It can be custom-defined by this item's Parent.
    property var getNextPage: function() {
        if (!root.requestPending && !root.noMoreDataToRetrieve) {
            root.requestPending = true;
            root.currentPageToRetrieve++;
            root.getPage();
            console.log("Fetching Page " + root.currentPageToRetrieve + " of " + root.listModelName + "...");
        }
    }
    
    // Resets both internal `ListModel`s and resets the page to retrieve to "1".
    function resetModel() {
        pagesAlreadyAdded = new Array();
        tempModel.clear();
        finalModel.clear();
        root.currentPageToRetrieve = 1;
    }

    onEndpointChanged: {
        resetModel();
        root.getPage();
    }

    onSortKeyChanged: {
        resetModel();
        root.getPage();
    }

    onSearchFilterChanged: {
        resetModel();
        root.getPage();
    }

    onTagsFilterChanged: {
        resetModel();
        root.getPage();
    }

    property bool initialResultReceived: false;
    property bool requestPending: false;
    property bool noMoreDataToRetrieve: false;
    property var pagesAlreadyAdded: new Array();
    
    // Redefining members and methods so that the parent of this Item
    // can use PSFListModel as they would a regular ListModel
    property alias model: finalModel;
    property alias count: finalModel.count;
    function clear() { finalModel.clear(); }
    function get(index) { return finalModel.get(index); }
    function remove(index) { return finalModel.remove(index); }
    function setProperty(index, prop, value) { return finalModel.setProperty(index, prop, value); }
    function move(from, to, n) { return finalModel.move(from, to, n); }

    // Used while processing page data and sorting
    ListModel {
        id: tempModel;
    }

    // This is the model that the parent of this Item will actually see
    ListModel {
        id: finalModel;
    }

    function processResult(status, retrievedResult) {
        root.initialResultReceived = true;
        root.requestPending = false;

        if (status === 'success') {
            var currentPage = parseInt(retrievedResult.current_page);

            if (retrievedResult.length === 0) {
                root.noMoreDataToRetrieve = true;
                console.log("No more data to retrieve from backend endpoint.")
            }
            /*
            See FIXME below...

            else if (root.currentPageToRetrieve === 1) {
                var sameItemCount = 0;

                tempModel.clear();
                tempModel.append(retrievedResult);
        
                for (var i = 0; i < tempModel.count; i++) {
                    if (!finalModel.get(i)) {
                        sameItemCount = -1;
                        break;
                    }
                    // Gotta think of a generic way to determine if the data we just got is the same
                    //     as the data that we already have in the model.
                    else if (tempModel.get(i).transaction_type === finalModel.get(i).transaction_type &&
                    tempModel.get(i).text === finalModel.get(i).text) {
                        sameItemCount++;
                    }
                }

                if (sameItemCount !== tempModel.count) {
                    finalModel.clear();
                    for (var i = 0; i < tempModel.count; i++) {
                        finalModel.append(tempModel.get(i));
                    }
                }
            }
            */
            else {
                // FIXME! Reconsider this logic, because it means that auto-refreshing the first page of results
                // (like we do in WalletHome for Recent Activity) _won't_ catch brand new data elements!
                // See the commented code above for how I did this for WalletHome specifically.
                if (root.pagesAlreadyAdded.indexOf(currentPage) !== -1) {
                    console.log("Page " + currentPage + " of paginated data has already been added to the list.");
                } else {
                    // First, add the result to a temporary model
                    tempModel.clear();
                    tempModel.append(retrievedResult);

                    // Make a note that we've already added this page to the model...
                    root.pagesAlreadyAdded.push(currentPage);

                    var insertionIndex = 0;
                    // If there's nothing in the model right now, we don't need to modify insertionIndex.
                    if (finalModel.count !== 0) {
                        var currentIteratorPage;
                        // Search through the whole model and look for the insertion point.
                        // The insertion point is found when the result page from the server is less than
                        //     the page that the current item came from, OR when we've reached the end of the whole model.
                        for (var i = 0; i < finalModel.count; i++) {
                            currentIteratorPage = finalModel.get(i).resultIsFromPage;
                        
                            if (currentPage < currentIteratorPage) {
                                insertionIndex = i;
                                break;
                            } else if (i === finalModel.count - 1) {
                                insertionIndex = i + 1;
                                break;
                            }
                        }
                    }
                    
                    // Go through the results we just got back from the server, setting the "resultIsFromPage"
                    //     property of those results and adding them to the main model.
                    // NOTE that this wouldn't be necessary if we did this step (or a similar step) on the server.
                    for (var i = 0; i < tempModel.count; i++) {
                        tempModel.setProperty(i, "resultIsFromPage", currentPage);
                        finalModel.insert(i + insertionIndex, tempModel.get(i))
                    }
                }
            }
            return true;
        } else {
            console.log("Failed to get page result for " + root.listModelName);
        }

        return false;
    }

    // Used when sorting model data on the CLIENT
    // Right now, there is no sorting done on the client for
    // any users of PSFListModel, but that could very easily change.
    property string sortColumnName: "";
    property bool isSortingDescending: true;
    property bool valuesAreNumerical: false;

    function swap(a, b) {
        if (a < b) {
            move(a, b, 1);
            move(b - 1, a, 1);
        } else if (a > b) {
            move(b, a, 1);
            move(a - 1, b, 1);
        }
    }

    function partition(begin, end, pivot) {
        if (valuesAreNumerical) {
            var piv = get(pivot)[sortColumnName];
            swap(pivot, end - 1);
            var store = begin;
            var i;

            for (i = begin; i < end - 1; ++i) {
                var currentElement = get(i)[sortColumnName];
                if (isSortingDescending) {
                    if (currentElement > piv) {
                        swap(store, i);
                        ++store;
                    }
                } else {
                    if (currentElement < piv) {
                        swap(store, i);
                        ++store;
                    }
                }
            }
            swap(end - 1, store);

            return store;
        } else {
            var piv = get(pivot)[sortColumnName].toLowerCase();
            swap(pivot, end - 1);
            var store = begin;
            var i;

            for (i = begin; i < end - 1; ++i) {
                var currentElement = get(i)[sortColumnName].toLowerCase();
                if (isSortingDescending) {
                    if (currentElement > piv) {
                        swap(store, i);
                        ++store;
                    }
                } else {
                    if (currentElement < piv) {
                        swap(store, i);
                        ++store;
                    }
                }
            }
            swap(end - 1, store);

            return store;
        }
    }

    function qsort(begin, end) {
        if (end - 1 > begin) {
            var pivot = begin + Math.floor(Math.random() * (end - begin));

            pivot = partition(begin, end, pivot);

            qsort(begin, pivot);
            qsort(pivot + 1, end);
        }
    }

    function quickSort() {
        qsort(0, count)
    }
}