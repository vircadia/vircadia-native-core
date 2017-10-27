#include "NotFilter.h"

using namespace controller;

NotFilter::NotFilter() {
}

float NotFilter::apply(float value) const {
    return (value == 0.0f) ? 1.0f : 0.0f;
}
