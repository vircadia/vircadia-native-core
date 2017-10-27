//
//  WebEntityView.qml
//
//  Created by Kunal Gosar on 16 March 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import "."

WebView {
    viewProfile: FileTypeProfile;
    
    urlTag: "noDownload=true";
}
