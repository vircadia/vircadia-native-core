//
//  Created by Ryan Huffman on 2016-12-14
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Profile.h"


#if defined(NSIGHT_FOUND)
#include "nvToolsExt.h"
#define NSIGHT_TRACING
#endif

Duration::Duration(const QLoggingCategory& category, const QString& name, uint32_t argbColor, uint64_t payload, QVariantMap args) : _name(name), _category(category) {
    if (_category.isDebugEnabled()) {
        args["nv_payload"] = QVariant::fromValue(payload);
        tracing::traceEvent(_category, _name, tracing::DurationBegin, "", args);

#if defined(NSIGHT_TRACING)
        nvtxEventAttributes_t eventAttrib { 0 };
        eventAttrib.version = NVTX_VERSION;
        eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
        eventAttrib.colorType = NVTX_COLOR_ARGB;
        eventAttrib.color = argbColor;
        eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
        eventAttrib.message.ascii = name.toUtf8().data();
        eventAttrib.payload.llValue = payload;
        eventAttrib.payloadType = NVTX_PAYLOAD_TYPE_UNSIGNED_INT64;

        nvtxRangePushEx(&eventAttrib);
#endif
    }
}

Duration::~Duration() {
    if (_category.isDebugEnabled()) {
        tracing::traceEvent(_category, _name, tracing::DurationEnd);
#ifdef NSIGHT_TRACING
        nvtxRangePop();
#endif
    }
}

// FIXME
uint64_t Duration::beginRange(const QLoggingCategory& category, const char* name, uint32_t argbColor) {
#ifdef NSIGHT_TRACING
    if (category.isDebugEnabled()) {
        nvtxEventAttributes_t eventAttrib = { 0 };
        eventAttrib.version = NVTX_VERSION;
        eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
        eventAttrib.colorType = NVTX_COLOR_ARGB;
        eventAttrib.color = argbColor;
        eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
        eventAttrib.message.ascii = name;
        return nvtxRangeStartEx(&eventAttrib);
    }
#endif
    return 0;
}

// FIXME
void Duration::endRange(const QLoggingCategory& category, uint64_t rangeId) {
#ifdef NSIGHT_TRACING
    if (category.isDebugEnabled()) {
        nvtxRangeEnd(rangeId);
    }
#endif
}

