#include "NotFilter.h"

using namespace controller;

NotFilter::NotFilter() {
}

AxisValue NotFilter::apply(AxisValue value) const {
    return { (value.value == 0.0f) ? 1.0f : 0.0f, value.timestamp, value.valid };
}
