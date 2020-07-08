module.exports = {
    root: true,
    extends: "eslint:recommended",
    "parserOptions": {
        "ecmaVersion": 5
    },

    "rules": {
        "brace-style": ["error", "1tbs", { "allowSingleLine": false }],
        "camelcase": ["error"],
        "comma-dangle": ["error", "never"],
        "curly": ["error", "all"],
        "eqeqeq": ["error", "always"],
        "indent": ["error", 4, { "SwitchCase": 1 }],
        "key-spacing": ["error", { "beforeColon": false, "afterColon": true, "mode": "strict" }],
        "keyword-spacing": ["error", { "before": true, "after": true }],
        "max-len": ["error", 128, 4],
        "new-cap": ["error"],
        "no-console": ["off"],
        "no-floating-decimal": ["error"],
        "no-magic-numbers": ["error", { "ignore": [0.5, -1, 0, 1, 2], "ignoreArrayIndexes": true }],
        "no-multi-spaces": ["error"],
        "no-multiple-empty-lines": ["error"],
        "no-unused-vars": ["error", { "args": "none", "vars": "local" }],
        "semi": ["error", "always"],
        "space-before-blocks": ["error"],
        "space-before-function-paren": ["error", { "anonymous": "ignore", "named": "never" }],
        "spaced-comment": ["error", "always", { "line": { "markers": ["/"] } }]
    }

};
