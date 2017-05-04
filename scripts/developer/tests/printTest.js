/* eslint-env jasmine */

// this test generates sample print, Script.print, etc. output

main();

function main() {
    // to match with historical behavior, Script.print(message) output only triggers
    //   the printedMessage signal (and therefore doesn't show up in the application log)
    Script.print('[Script.print] hello world');

    // the rest of these should show up in both the application log and signaled print handlers
    print('[print]', 'hello', 'world');

    // note: these trigger the equivalent of an emit
    Script.printedMessage('[Script.printedMessage] hello world', '{filename}');
    Script.infoMessage('[Script.infoMessage] hello world', '{filename}');
    Script.warningMessage('[Script.warningMessage] hello world', '{filename}');
    Script.errorMessage('[Script.errorMessage] hello world', '{filename}');

    {
        // FIXME: these only show up in the application debug log
        Vec3.print('[Vec3.print]', Vec3.HALF);

        var q = Quat.fromPitchYawRollDegrees(45, 45, 45);
        Quat.print('[Quat.print]', q);

        var m = Mat4.createFromRotAndTrans(q, Vec3.HALF);
        Mat4.print('[Mat4.print (row major)]', m);
    }
}
