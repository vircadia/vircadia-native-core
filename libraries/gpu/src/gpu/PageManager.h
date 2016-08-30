//
//  Created by Sam Gateau on 10/8/2014.
//  Split from Resource.h/Resource.cpp by Bradley Austin Davis on 2016/08/07
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_PageManager_h
#define hifi_gpu_PageManager_h

#include "Forward.h"

#include <vector>

namespace gpu {

struct PageManager {
    static const Size DEFAULT_PAGE_SIZE = 4096;

    // Currently only one flag... 'dirty'
    enum Flag {
        DIRTY = 0x01,
    };

    using FlagType = uint8_t;

    // A list of flags
    using Vector = std::vector<FlagType>;
    // A list of pages
    using Pages = std::vector<Size>;

    Vector _pages;
    uint8 _flags{ 0 };
    const Size _pageSize;

    PageManager(Size pageSize = DEFAULT_PAGE_SIZE);
    PageManager& operator=(const PageManager& other);

    operator bool() const;
    bool operator()(uint8 desiredFlags) const;
    void markPage(Size index, uint8 markFlags = DIRTY);
    void markRegion(Size offset, Size bytes, uint8 markFlags = DIRTY);
    Size getPageCount(uint8_t desiredFlags = DIRTY) const;
    Size getSize(uint8_t desiredFlags = DIRTY) const;
    void setPageCount(Size count);
    Size getRequiredPageCount(Size size) const;
    Size getRequiredSize(Size size) const;
    Size accommodate(Size size);
    // Get pages with the specified flags, optionally clearing the flags as we go
    Pages getMarkedPages(uint8_t desiredFlags = DIRTY, bool clear = true);
    bool getNextTransferBlock(Size& outOffset, Size& outSize, Size& currentPage);
};

};

#endif
