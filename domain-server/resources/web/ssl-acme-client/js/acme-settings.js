import {showLoadingScreen} from "./utils.js"
import {
    getSettings, postSettings, getAcmeMeta,
    getZeroSSLEebFromApiKey, getZeroSSLEebFromEmail
} from "./api.js"

const directorySelect = document.getElementById("directory-url-select");
const directoryInput = document.getElementById("directory-url");
const termsOfServiceLink = document.getElementById("terms-of-service");
const authSelect = document.getElementById("auth-select");
const controlsContainer = document.getElementById("control-button-container");
const saveButton = document.getElementById("save-button");

const zeroSSlAuthInput = document.getElementById("zero-ssl-auth-input");
const eabKidInput = document.getElementById("eab-kid-input");
const eabMacInput = document.getElementById("eab-mac-input");

function update() {
    directoryInput.hidden = directorySelect.value !== "custom";
    const authType = authSelect.value;
    let authPlaceholder = "ZeroSSL API Key";
    switch(authType) {
        case "account-key-only":
            zeroSSlAuthInput.hidden = true;
            eabKidInput.hidden = true;
            eabMacInput.hidden = true;
            break;

        case "zero-ssl-email":
            authPlaceholder = "ZeroSSL Email";
        case "zero-ssl-api-key":
            zeroSSlAuthInput.placeholder = authPlaceholder;
            zeroSSlAuthInput.hidden = false;
            eabKidInput.hidden = false;
            eabMacInput.hidden = false;
            eabKidInput.disabled = true;
            eabMacInput.disabled = true;
            break;
        case "id-mac":
            zeroSSlAuthInput.hidden = true;
            eabKidInput.hidden = false;
            eabMacInput.hidden = false;
            eabKidInput.disabled = false;
            eabMacInput.disabled = false;
            break;
    }
}

function getDirectoryUrl() {
    return directorySelect.value !== "custom"
        ? directorySelect.value
        : directoryInput.value;
}

function updateStatus() {
    updateDirectoryMeta();
}

function updateDirectoryMeta() {
    showLoadingScreen(true);
    getAcmeMeta(getDirectoryUrl()).then(meta => {
        const zeroSSLSelected = directorySelect.selectedOptions[0].text === "ZeroSSL";
        for (const authOption of authSelect.options) {
            const classes = Array.from(authOption.classList);
            if (classes.includes("eab-option")) {
                authOption.disabled = !meta || !meta.externalAccountRequired;
            }
            if (classes.includes("zero-ssl-auth-option")) {
                if (zeroSSLSelected) {
                    if (classes.includes("zero-ssl-auth-option-default")) {
                        authOption.selected = true;
                    }
                } else {
                    authOption.disabled = true;
                }
            }
            if(authOption.disabled && authOption.selected) authOption.selected = false;
        }
        termsOfServiceLink.href = meta && meta.termsOfService || "404.html";
        update();
    }).finally(() => {
        showLoadingScreen(false);
    });
}

directorySelect.addEventListener("change", () => {
    updateDirectoryMeta();
});

function getEabFromZeroSSL() {
    const authType = authSelect.value;
    const method = authType === "zero-ssl-email" ? getZeroSSLEebFromEmail :
        authType === "zero-ssl-api-key" ? getZeroSSLEebFromApiKey :
        undefined;
    if (method) {
        showLoadingScreen(true);
        method(zeroSSlAuthInput.value)
            .then((json) => {
                if (json.success) {
                    eabKidInput.value = json.eab_kid;
                    eabMacInput.value = json.eab_hmac_key;
                } else {
                    eabKidInput.value = "";
                    eabMacInput.value = "";
                }
            }).finally(() => {
                showLoadingScreen(false);
                update();
            });
    } else {
        update();
    }
}

authSelect.addEventListener("change", () => {
    const authType = authSelect.value;
    if (authType === "account-key-only" || authType === "id-mac") {
        update();
    } else {
        getEabFromZeroSSL();
    }
});

let directoryInputTimeout = undefined;
directoryInput.addEventListener("input", () => {
    if(undefined !== directoryInputTimeout)
        clearTimeout(directoryInputTimeout);
    directoryInputTimeout = setTimeout(updateDirectoryMeta, 500);
});

let zeroSSlAuthInputTimeout = undefined;
zeroSSlAuthInput.addEventListener("input", () => {
    if(undefined !== zeroSSlAuthInputTimeout)
        clearTimeout(zeroSSlAuthInputTimeout);
    zeroSSlAuthInputTimeout = setTimeout(getEabFromZeroSSL, 500);
});

function updateSettings() {
    getSettings().then((settings) => {
        setDirectoryUrl(settings.values.acme.directory_endpoint);
        eabKidInput.value = settings.values.acme.eab_kid;
        eabMacInput.value = settings.values.acme.eab_mac;
        updateStatus();
    });
}

saveButton.addEventListener("click", () => {
    showLoadingScreen(true);
    postSettings({acme: {
        directory_endpoint: getDirectoryUrl(),
        eab_kid: eabKidInput.value,
        eab_mac: eabMacInput.value
        // TODO: ZeroSSL Authentication method and source (email or api key)
    } }).then(() => {
        updateSettings();
    }).finally(() => {
        showLoadingScreen(false);
    });
});

function setDirectoryUrl(url) {
    if (url === "custom") {
        directorySelect.value = "custom";
        directoryInput.value = "custom";
    } else {
        const option = Array.from(directorySelect.options)
            .find(o => o.value === url);
        if(undefined !== option) {
            directorySelect.value = option.value;
        } else {
            directorySelect.value = "custom";
            directoryInput.value = url;
        }
    }
}

updateSettings();
