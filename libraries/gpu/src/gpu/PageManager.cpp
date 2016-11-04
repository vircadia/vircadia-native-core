//
//  Created by Sam Gateau on 10/8/2014.
//  Split from Resource.h/Resource.cpp by Bradley Austin Davis on 2016/08/07
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "PageManager.h"

using namespace gpu;

PageManager::PageManager(Size pageSize) : _pageSize(pageSize) {}

PageManager& PageManager::operator=(const PageManager& other) {
    assert(other._pageSize == _pageSize);
    _pages = other._pages;
    _flags = other._flags;
    return *this;
}

PageManager::operator bool() const {
    return (*this)(DIRTY);
}

bool PageManager::operator()(uint8 desiredFlags) const {
    return (desiredFlags == (_flags & desiredFlags));
}

void PageManager::markPage(Size index, uint8 markFlags) {
    assert(_pages.size() > index);
    _pages[index] |= markFlags;
    _flags |= markFlags;
}

void PageManager::markRegion(Size offset, Size bytes, uint8 markFlags) {
    if (!bytes) {
        return;
    }
    _flags |= markFlags;
    // Find the starting page
    Size startPage = (offset / _pageSize);
    // Non-zero byte count, so at least one page is dirty
    Size pageCount = 1;
    // How much of the page is after the offset?
    Size remainder = _pageSize - (offset % _pageSize);
    //  If there are more bytes than page space remaining, we need to increase the page count
    if (bytes > remainder) {
        // Get rid of the amount that will fit in the current page
        bytes -= remainder;

        pageCount += (bytes / _pageSize);
        if (bytes % _pageSize) {
            ++pageCount;
        }
    }

    // Mark the pages dirty
    for (Size i = 0; i < pageCount; ++i) {
        _pages[i + startPage] |= markFlags;
    }
}

Size PageManager::getPageCount(uint8_t desiredFlags) const {
    Size result = 0;
    for (auto pageFlags : _pages) {
        if (desiredFlags == (pageFlags & desiredFlags)) {
            ++result;
        }
    }
    return result;
}

Size PageManager::getSize(uint8_t desiredFlags) const {
    return getPageCount(desiredFlags) * _pageSize;
}

void PageManager::setPageCount(Size count) {
    _pages.resize(count);
}

Size PageManager::getRequiredPageCount(Size size) const {
    Size result = size / _pageSize;
    if (size % _pageSize) {
        ++result;
    }
    return result;
}

Size PageManager::getRequiredSize(Size size) const {
    return getRequiredPageCount(size) * _pageSize;
}

Size PageManager::accommodate(Size size) {
    Size newPageCount = getRequiredPageCount(size);
    Size newSize = newPageCount * _pageSize;
    _pages.resize(newPageCount, 0);
    return newSize;
}

// Get pages with the specified flags, optionally clearing the flags as we go
PageManager::Pages PageManager::getMarkedPages(uint8_t desiredFlags, bool clear) {
    Pages result;
    if (desiredFlags == (_flags & desiredFlags)) {
        _flags &= ~desiredFlags;
        result.reserve(_pages.size());
        for (Size i = 0; i < _pages.size(); ++i) {
            if (desiredFlags == (_pages[i] & desiredFlags)) {
                result.push_back(i);
                if (clear) {
                    _pages[i] &= ~desiredFlags;
                }
            }
        }
    }
    return result;
}

bool PageManager::getNextTransferBlock(Size& outOffset, Size& outSize, Size& currentPage) {
    Size pageCount = _pages.size();
    // Advance to the first dirty page
    while (currentPage < pageCount && (0 == (DIRTY & _pages[currentPage]))) {
        ++currentPage;
    }

    // If we got to the end, we're done
    if (currentPage >= pageCount) {
        return false;
    }

    // Advance to the next clean page
    outOffset = static_cast<Size>(currentPage * _pageSize);
    while (currentPage < pageCount && (0 != (DIRTY & _pages[currentPage]))) {
        _pages[currentPage] &= ~DIRTY;
        ++currentPage;
    }
    outSize = static_cast<Size>((currentPage * _pageSize) - outOffset);
    return true;
}
