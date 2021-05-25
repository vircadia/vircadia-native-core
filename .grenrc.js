//
//  .grenrc.js
//
//  Created by Kalila L. on May 25, 2021
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  This configuration is for generating a changelog with gren for a GitHub repository.
//
//  gren changelog -G
//

module.exports = {
    "dataSource": "prs",
    "prefix": "",
    "ignoreLabels": [
        "enhancement",
        "bugfix",
        "CR Approved",
        "QA Approved",
        "allow-build-upload",
        "bug",
        "confirmed",
        "do not merge",
        "duplicate",
        "good first issue",
        "help wanted",
        "hifi migration",
        "high risk",
        "rebuild",
        "merge right before snip"
    ],
    "onlyMilestones": true,
    "groupBy": {
        "Enhancements": ["enhancement"],
        "Bug Fixes": ["bugfix"],
        "Docs": ["docs"]
    },
    "changelogFilename": "CHANGELOG.md"
}