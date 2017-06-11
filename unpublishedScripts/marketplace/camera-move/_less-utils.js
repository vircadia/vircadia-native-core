/* eslint-env jquery, browser */
/* eslint-disable comma-dangle */
/* global PARAMS, signal, assert, log, debugPrint */

function preconfigureLESS(options) {
    assert(function assertion() {
        return options.selector && options.globalVars;
    });

    options.globalVars = Object.assign(options.globalVars||{}, { hash: '' });
    Object.assign(options, {
        // poll: 1000,
        // watch: true,
    });
    var lessManager = {
        options: options,
        onViewportUpdated: onViewportUpdated,
        elements: $(options.selector).remove(),
    };

    return lessManager;

    function onViewportUpdated(viewport) {
        var globalVars = options.globalVars,
            less = window.less;
        if (onViewportUpdated.to) {
            clearTimeout(onViewportUpdated.to);
            onViewportUpdated.to = 0;
        } else if (globalVars.hash) {
            onViewportUpdated.to = setTimeout(onViewportUpdated.bind(this, viewport), 500); // debounce
            return;
        }
        delete globalVars.hash;
        Object.assign(globalVars, {
            'interface-mode': /highfidelity/i.test(navigator.userAgent),
            'inner-width': viewport.inner.width,
            'inner-height': viewport.inner.height,
            'client-width': viewport.client.width,
            'client-height': viewport.client.height,
            'hash': '',
        });
        globalVars.hash = JSON.stringify(JSON.stringify(globalVars,0,2)).replace(/\\n/g , '\\000a');
        var hash = JSON.stringify(globalVars, 0, 2);
        log('onViewportUpdated', JSON.parse(onViewportUpdated.lastHash||'{}')['inner-width'], JSON.parse(hash)['inner-width']);
        if (onViewportUpdated.lastHash !== hash) {
            debugPrint('updating lessVars', 'less.modifyVars:' + typeof less.modifyVars, JSON.stringify(globalVars, 0, 2));
            PARAMS.lessDebug && $('#errors').show().html("<pre>").children(0).text(hash);
            // LESS needs some help to recompile inline styles, so a fresh copy of the source nodes is swapped-in
            var newNodes = lessManager.elements.clone().appendTo(document.body);
            // less.modifyVars && less.modifyVars(true, globalVars);
            // note: refresh(reload, modifyVars, clearFileCache)
            less.refresh(false, globalVars).then(function(result) {
                log('less.refresh completed OK', result);
            })['catch'](function(err) {
                log('less ERROR:', err);
            });
            var oldNodes = onViewportUpdated.lastNodes;
            oldNodes && oldNodes.remove();
            onViewportUpdated.lastNodes = newNodes;
        }
        onViewportUpdated.lastHash = hash;
    }
}
