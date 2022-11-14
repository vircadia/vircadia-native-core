//
//  utils.js
//  domain-server/resources/web/ssl-acme-client/js
//
//  Created by Nshan G. on 2021-12-26
//  Copyright 2021 Vircadia contributors.
//  Copyright 2021 DigiSomni LLC.
//  Copyright 2021 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function showLoadingScreen(shown) {
    //TODO: proper loading UI
    document.body.hidden = shown;
}

export { showLoadingScreen };
