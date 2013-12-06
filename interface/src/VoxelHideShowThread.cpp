//
//  VoxelHideShowThread.cpp
//  interface
//
//  Created by Brad Hefta-Gaub on 12/1/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded interface thread for hiding and showing voxels in the local tree.
//

#include <QDebug>
#include <NodeList.h>
#include <PerfStat.h>
#include <SharedUtil.h>

#include "Menu.h"
#include "VoxelHideShowThread.h"

VoxelHideShowThread::VoxelHideShowThread(VoxelSystem* theSystem) :
    _theSystem(theSystem) {
}

bool VoxelHideShowThread::process() {
    const uint64_t MSECS_TO_USECS = 1000;
    const uint64_t SECS_TO_USECS = 1000 * MSECS_TO_USECS;
    const uint64_t FRAME_RATE = 60;
    const uint64_t USECS_PER_FRAME = SECS_TO_USECS / FRAME_RATE; // every 60fps

    uint64_t start = usecTimestampNow();
    _theSystem->checkForCulling();
    uint64_t end = usecTimestampNow();
    uint64_t elapsed = end - start;
    
    bool showExtraDebugging = Menu::getInstance()->isOptionChecked(MenuOption::ExtraDebugging);
    if (showExtraDebugging && elapsed > USECS_PER_FRAME) {
        printf("VoxelHideShowThread::process()... checkForCulling took %llu\n", elapsed);
    }
    
    if (isStillRunning()) {
        if (elapsed < USECS_PER_FRAME) {
            uint64_t sleepFor = USECS_PER_FRAME - elapsed;
            usleep(sleepFor);
        }
    }    
    return isStillRunning();  // keep running till they terminate us
}
