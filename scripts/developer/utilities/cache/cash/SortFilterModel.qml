import QtQuick 2.9
import QtQml.Models 2.3

DelegateModel {
    id: delegateModel

    property var lessThan: function(left, right) { return true; }
    property var acceptItem: function(item) { return true; }

    function insertPosition(lessThanFunctor, item) {
        var lower = 0
        var upper = visibleItems.count
        while (lower < upper) {
            var middle = Math.floor(lower + (upper - lower) / 2)
            var result = lessThanFunctor(item.model, visibleItems.get(middle).model);
            if (result) {
                upper = middle
            } else {
                lower = middle + 1
            }
        }
        return lower
    }

    function sortAndFilter(lessThanFunctor, acceptItemFunctor) {
        while (unsortedItems.count > 0) {
            var item = unsortedItems.get(0)
            
            if (acceptItemFunctor(item.model)) {
                var index = insertPosition(lessThanFunctor, item)

                item.groups = ["items","visible"]
                visibleItems.move(item.visibleIndex, index)
            } else {
                item.groups = ["items"]
            }
        }
    }

    // Private bool to track when items changed and view is dirty
    property bool itemsDirty: true
    function update() {
        console.log("SortFilterMode: update and sort and filter items !!" + items.count);
        if (items.count > 0) {
            items.setGroups(0, items.count, ["items","unsorted"]);
        }

        sortAndFilter(lessThan, acceptItem)
        itemsDirty = false;
        itemsUpdated()
    }

    signal itemsUpdated()

    function updateOnItemsChanged() {
        itemsDirty = true;
        if (isAutoUpdateOnChanged) {
            update()
        }
    }

    property bool isAutoUpdateOnChanged: false
    function setAutoUpdateOnChanged(enabled) {
        isAutoUpdateOnChanged = enabled
        if (enabled) {
            update()
        }
    }

    function forceUpdate() {
        update();
    }
    items.onChanged: updateOnItemsChanged()
    onLessThanChanged: update()
    onAcceptItemChanged: update()

    groups: [
        DelegateModelGroup {
            id: visibleItems

            name: "visible"
            includeByDefault: false
        },
        DelegateModelGroup {
            id: unsortedItems

            name: "unsorted"
            includeByDefault: false
        }
    ]

    filterOnGroup: "visible"
}