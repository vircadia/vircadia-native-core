//
//  SortableListModel.qml
//  qml/hifi/commerce/common
//
//  SortableListModel
//
//  Created by Zach Fox on 2017-09-28
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5

ListModel {
    id: root;
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