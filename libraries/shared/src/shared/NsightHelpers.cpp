//
//  Created by Bradley Austin Davis on 2015/12/10
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "NsightHelpers.h"


#if defined(_WIN32) && defined(NSIGHT_FOUND)

#include <QtCore/QProcessEnvironment>
#include "nvToolsExt.h"

static const QString NSIGHT_FLAG("NSIGHT_LAUNCHED");
static const bool nsightLaunched = QProcessEnvironment::systemEnvironment().contains(NSIGHT_FLAG);

bool nsightActive() {
    return nsightLaunched;
}


uint64_t ProfileRange::beginRange(const char* name, uint32_t argbColor) {
    nvtxEventAttributes_t eventAttrib = { 0 };
    eventAttrib.version = NVTX_VERSION;
    eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
    eventAttrib.colorType = NVTX_COLOR_ARGB;
    eventAttrib.color = argbColor;
    eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
    eventAttrib.message.ascii = name;
    return nvtxRangeStartEx(&eventAttrib);
  //  return nvtxRangePushEx(&eventAttrib);
}

void ProfileRange::endRange(uint64_t rangeId) {
    nvtxRangeEnd(rangeId);
   // nvtxRangePop();
}

ProfileRange::ProfileRange(const char *name) {
   // _rangeId = nvtxRangeStart(name);
    _rangeId = nvtxRangePush(name);
}

ProfileRange::ProfileRange(const char *name, uint32_t argbColor, uint64_t payload) {
    nvtxEventAttributes_t eventAttrib = {0};
    eventAttrib.version = NVTX_VERSION;
    eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
    eventAttrib.colorType = NVTX_COLOR_ARGB;
    eventAttrib.color = argbColor;
    eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
    eventAttrib.message.ascii = name;
    eventAttrib.payload.llValue = payload;
    eventAttrib.payloadType = NVTX_PAYLOAD_TYPE_UNSIGNED_INT64;

    //_rangeId = nvtxRangeStartEx(&eventAttrib);
    _rangeId = nvtxRangePushEx(&eventAttrib);
}

ProfileRange::~ProfileRange() {
   // nvtxRangeEnd(_rangeId);
    nvtxRangePop();
}

#else

bool nsightActive() {
    return false;
}

#endif // _WIN32
