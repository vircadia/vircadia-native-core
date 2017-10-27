/* eslint-env jquery, browser */
/* eslint-disable comma-dangle */
/* global PARAMS, signal, assert, log, debugPrint */

function preconfigureLESS(options) {
    assert(function assertion() {
        return options.selector && options.globalVars;
    });

    options.globalVars = Object.assign(options.globalVars||{}, { hash: '' });
    Object.assign(options, {
        errorReporting: Object.assign(function errorReporting(op, e, rootHref) {
            var type = 'Less-'+e.type+'-Error';
            var lastResult = errorReporting.lastResult;
            delete errorReporting.lastResult;
            if (lastResult && lastResult['#line']) {
                e.line += lastResult['#line'] - 2;
            }
            e.stack = [type+':'+e.message+' at '+e.filename+':'+(e.line+1)]
                .concat((e.extract||[]).map(function(output, index) {
                    return (index + e.line)+': '+output;
                })).join('\n\t');
            _debug.handleUncaughtException(type+': ' + e.message, e.filename, e.line, e.column, {
                type: type,
                message: e.message,
                fileName: e.filename,
                lineNumber: e.line,
                stack: e.stack,
            });
        }, {
            lastResult: null,
            lastSource: null,
            getLocation: null,
            $patch: function() {
                // when errors occur, stash the source and #line {offset} for better error dispay above
                assert(!this.getLocation);
                var originalGetLocation = less.utils.getLocation,
                    self = this;
                less.utils.getLocation = customGetLocation;
                delete self.$patch; // only need to apply once
                function customGetLocation(index, stream) {
                    self.lastSource = stream;
                    var result = originalGetLocation.apply(this, arguments);
                    result.source = stream;
                    stream.replace(/#line (\d+)/, function(_, offset) {
                        result['#line'] = parseInt(offset);
                    });
                    return self.lastResult = result;
                }
            },
        }),
        // poll: 1000,
        // watch: true,
    });
    var lessManager = {
        options: options,
        onViewportUpdated: onViewportUpdated,
        elements: $(options.selector).remove()
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
            'inner-width': viewport.inner.width,
            'inner-height': viewport.inner.height,
            'client-width': viewport.client.width,
            'client-height': viewport.client.height,
            'hash': '',
        });
        globalVars.hash = JSON.stringify(JSON.stringify(globalVars,0,2)).replace(/\\n/g , '\\000a');
        var hash = JSON.stringify(globalVars, 0, 2);
        debugPrint('onViewportUpdated', JSON.parse(onViewportUpdated.lastHash||'{}')['inner-width'], JSON.parse(hash)['inner-width']);
        if (onViewportUpdated.lastHash !== hash) {
            debugPrint('updating lessVars', 'less.modifyVars:' + typeof less.modifyVars, JSON.stringify(globalVars, 0, 2));
            // dump less variables if lessDebug=true was in the url
            PARAMS.lessDebug && $('#errors').show().html("<pre>").children(0).text(hash);

            // patch less with absolute line number reporting
            options.errorReporting && options.errorReporting.$patch && options.errorReporting.$patch();

            // to recompile inline styles (in response to onresize or when developing),
            // a fresh copy of the source nodes gets swapped-in
            var newNodes = lessManager.elements.clone().appendTo(document.body);
            // note: refresh(reload, modifyVars, clearFileCache)
            less.refresh(false, globalVars).then(function(result) {
                debugPrint('less.refresh completed OK', result);
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
