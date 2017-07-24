/* eslint-env jquery, browser */
/* eslint-disable comma-dangle, no-empty */
/* global _utils, PARAMS, VERSION, signal, assert, log, debugPrint,
          bridgedSettings, POPUP */

// JSON export / import helpers proto module
var SettingsJSON = (function() {
    _utils.exists;
    assert.exists;

    return {
        setPath: setPath,
        rollupPaths: rollupPaths,
        encodeNodes: encodeNodes,
        exportAll: exportAll,
        showSettings: showSettings,
        applyJSON: applyJSON,
        promptJSON: promptJSON,
        popupJSON: popupJSON,
    };

    function encodeNodes(resolver) {
        return resolver.getAllNodes().reduce((function(out, input, i) {
            //debugPrint('input['+i+']', input.id);
            var id = input.id,
                key = resolver.getKey(id);
            //debugPrint('toJSON', id, key, input.id);
            setPath(out, key.split('/'), resolver.getValue(key));
            return out;
        }).bind(this), {});
    }

    function setPath(obj, path, value) {
        var key = path.pop();
        obj = path.reduce(function(obj, subkey) {
            return obj[subkey] = obj[subkey] || {};
        }, obj);
        //debugPrint('setPath', key, Object.keys(obj));
        obj[key] = value;
    }

    function rollupPaths(obj, output, path) {
        path = path || [];
        output = output || {};
        // log('rollupPaths', Object.keys(obj||{}), Object.keys(output), path);
        for (var p in obj) {
            path.push(p);
            var value = obj[p];
            if (value && typeof value === 'object') {
                rollupPaths(obj[p], output, path);
            } else {
                output[path.join('/')] = value;
            }
            path.pop();
        }
        return output;
    }

    function exportAll(resolver, name) {
        var settings = encodeNodes(resolver);
        Object.keys(settings).forEach(function(prop) {
            if (typeof settings[prop] === 'object') {
                _utils.sortedAssign(settings[prop]);
            }
        });
        return {
            version: VERSION,
            name: name || undefined,
            settings: settings,
            _metadata: { timestamp: new Date(), PARAMS: PARAMS, url: location.href, }
        };
    }

    function showSettings(resolver, saveName) {
        popupJSON(saveName || '(current settings)', Object.assign(exportAll(resolver, saveName), {
            extraParams: bridgedSettings.extraParams,
        }));
    }

    function popupJSON(title, tmp) {
        var HTML = document.getElementById('POPUP').innerHTML
            .replace(/\bxx-script\b/g, 'script')
            .replace('JSON', JSON.stringify(tmp, 0, 2).replace(/\n/g, '<br />'));
        if (/WebWindowEx/.test(navigator.userAgent) ) {
            bridgedSettings.sendEvent({
                method: 'overlayWebWindow',
                userAgent: navigator.userAgent,
                options: {
                    title: 'app-camera-move-export' + (title ? '::'+title : ''),
                    content: HTML,
                },
            });
        } else {
            // append a footer to the data URI so it displays cleaner in the built-in browser window that opens
            var footer = '<\!-- #' + HTML.substr(0,256).replace(/./g,' ') + (title || 'Camera Move Settings');
            window.open("data:text/html;escape," + encodeURIComponent(HTML) + footer,"app-camera-move-export");
        }
    }

    function applyJSON(resolver, name, tmp) {
        assert(tmp && 'version' in tmp && 'settings' in tmp, 'invalid settings record: ' + JSON.stringify(tmp));
        var settings = rollupPaths(tmp.settings);
        for (var p in settings) {
            if (/^[.]/.test(p)) {
                continue;
            }
            var key = resolver.getId(p, true);
            if (!key) {
                log('$applySettings -- skipping unregistered Settings key: ', p);
            } else {
                resolver.setValue(p, settings[p], name+'.settings.'+p);
            }
        }
    }

    function promptJSON() {
        var json = window.prompt('(paste JSON here)', '');
        if (!json) {
            return;
        }
        try {
            log('parsing json', json);
            json = JSON.parse(json);
        } catch (e) {
            throw new Error('Could not parse pasted JSON: ' + e + '\n\n' + (json||'').replace(/</g,'&lt;'));
        }
        return json;
    }
})(this);

