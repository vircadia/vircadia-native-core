//
//  Created by Bradley Austin Davis 2016/01/20
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Preferences.h"


void Preferences::addPreference(Preference* preference) {
    const QString& category = preference->getCategory();
    QVariantList categoryPreferences;
    // FIXME is there an easier way to do this with less copying?
    if (_preferencesByCategory.contains(category)) {
        categoryPreferences = qvariant_cast<QVariantList>(_preferencesByCategory[category]);
    } else {
        // Use this property to maintain the order of the categories
        _categories.append(category);
    }
    categoryPreferences.append(QVariant::fromValue(preference));
    _preferencesByCategory[category] = categoryPreferences;
}

