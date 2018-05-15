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
    property string sortColumnName: "";
    property bool isSortingDescending: true;
    property bool valuesAreNumerical: false;

    property bool initialResultReceived: false;
    property bool requestPending: false;
    property bool noMoreDataToRetrieve: false;
    property int nextPageToRetrieve: 1;
    property var pagesAlreadyAdded: new Array();

    ListModel {
        id: tempModel;
    }

    function processResult(status, retrievedResult) {
        root.initialResultReceived = true;
        root.requestPending = false;

        if (status === 'success') {
            var currentPage = parseInt(result.current_page);

            if (retrievedResult.length === 0) {
                root.noMoreDataToRetrieve = true;
                console.log("No more data to retrieve from backend endpoint.")
            } else if (root.nextPageToRetrieve === 1) {
                var sameItemCount = 0;

                tempModel.clear();
                tempModel.append(retrievedResult);
        
                for (var i = 0; i < tempModel.count; i++) {
                    if (!root.get(i)) {
                        sameItemCount = -1;
                        break;
                    }
                    // Gotta think of another way to determine if the data we just got is the same
                    //     as the data that we already have in the model.
                    /* else if (tempModel.get(i).transaction_type === root.get(i).transaction_type &&
                    tempModel.get(i).text === root.get(i).text) {
                        sameItemCount++;
                    }*/
                }

                if (sameItemCount !== tempModel.count) {
                    root.clear();
                    for (var i = 0; i < tempModel.count; i++) {
                        root.append(tempModel.get(i));
                    }
                }
            } else {
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
                    if (root.count !== 0) {
                        var currentIteratorPage;
                        // Search through the whole model and look for the insertion point.
                        // The insertion point is found when the result page from the server is less than
                        //     the page that the current item came from, OR when we've reached the end of the whole model.
                        for (var i = 0; i < root.count; i++) {
                            currentIteratorPage = root.get(i).resultIsFromPage;
                        
                            if (currentPage < currentIteratorPage) {
                                insertionIndex = i;
                                break;
                            } else if (i === root.count - 1) {
                                insertionIndex = i + 1;
                                break;
                            }
                        }
                    }
                    
                    // Go through the results we just got back from the server, setting the "resultIsFromPage"
                    //     property of those results and adding them to the main model.
                    // NOTE that this wouldn't be necessary if we did this step on the server.
                    for (var i = 0; i < tempModel.count; i++) {
                        tempModel.setProperty(i, "resultIsFromPage", currentPage);
                        root.insert(i + insertionIndex, tempModel.get(i))
                    }
                }
            }
        }
    }

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