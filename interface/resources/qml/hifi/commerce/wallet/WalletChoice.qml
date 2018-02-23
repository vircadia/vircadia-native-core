//
//  WalletChoice.qml
//  qml/hifi/commerce/wallet
//
//  WalletChoice
//
//  Created by Howard Stearns
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.5
import "../../../styles-uit"

Rectangle {
    property string activeView: "conflict";
    RalewayRegular {
        text: "You have a wallet elsewhere." + activeView;
    }
}
