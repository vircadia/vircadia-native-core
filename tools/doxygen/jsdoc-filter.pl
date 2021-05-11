# Filter out JSDoc comments by transforming them to ordinary comments.
# (JSDoc comments are used to generate the user API docs and don't produce suitable results for the C++ docs.)
while (<>) {
    if (m/^\s*\/\*\*jsdoc/) {
        print "\/*\n";
    } else {
        print;
    }
}
