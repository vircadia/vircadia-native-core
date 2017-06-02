//
//  TranslationAccumulator.h
//
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TranslationAccumulator_h
#define hifi_TranslationAccumulator_h

#include <glm/glm.hpp>

class TranslationAccumulator {
public:
    TranslationAccumulator() : _accum(0.0f, 0.0f, 0.0f), _totalWeight(0), _isDirty(false) { }

    int size() const { return _totalWeight > 0.0f; }

    /// \param translation translation to add
    /// \param weight contribution factor of this translation to total accumulation
    void add(const glm::vec3& translation, float weight = 1.0f);

    glm::vec3 getAverage();

    /// \return true if any translation were accumulated
    bool isDirty() const { return _isDirty; }

    /// \brief clear accumulated translation but don't change _isDirty
    void clear();

    /// \brief clear accumulated translation and set _isDirty to false
    void clearAndClean();

private:
    glm::vec3 _accum;
    float _totalWeight;
    bool _isDirty;
};

#endif // hifi_TranslationAccumulator_h
