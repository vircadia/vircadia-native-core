//
//  Created by Bradley Austin Davis 2016/01/20
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Preferences.h"


void Preferences::addPreference(Preference* preference) {
    preference->setParent(this);

    const QString& category = preference->getCategory();

    // Use this structure to maintain the order of the categories
    if (!_categories.contains(category)) {
        _categories.append(category);
    }
    
    QVariantList categoryPreferences;
    // FIXME is there an easier way to do this with less copying?
    if (_preferencesByCategory.contains(category)) {
        categoryPreferences = qvariant_cast<QVariantList>(_preferencesByCategory[category]);
    } 
    categoryPreferences.append(QVariant::fromValue(preference));
    _preferencesByCategory[category] = categoryPreferences;
}

