function getSettings() {
    return fetch("/settings.json").then(response => response.json());
}

function postSettings(settings) {
    return fetch("/settings.json",
    {
        headers: {
          'Accept': 'application/json',
          'Content-Type': 'application/json'
        },
        method: "POST",
        body: JSON.stringify(settings)
    });
}

function getAcmeMeta(directory) {
    return fetch(directory).then(response => response.text())
        .then(text => {
            let json = {};
            try { json = JSON.parse(text); }
            catch(error) {}
            return json.meta;
        })
        .catch( () => undefined )
    ;
}

export { getSettings, postSettings, getAcmeMeta }
