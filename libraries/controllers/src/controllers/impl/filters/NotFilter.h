#pragma once
#ifndef hifi_Controllers_Filters_Not_h
#define hifi_Controllers_Filters_Not_h

#include "../Filter.h"

namespace controller {

class NotFilter : public Filter {
    REGISTER_FILTER_CLASS(NotFilter);
public:
    NotFilter();

    virtual float apply(float value) const override;
    virtual Pose apply(Pose value) const override { return value; }
};

}

#endif

