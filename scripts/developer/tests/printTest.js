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
        Vec3.print('[Vec3.print]', Vec3.HALF);

        var q = Quat.fromPitchYawRollDegrees(45, 45, 45);
        Quat.print('[Quat.print]', q);
        Quat.print('[Quat.print (euler)]', q, true);

        function vec4(x,y,z,w) {
            return { x: x, y: y, z: z, w: w };
        }
        var m = Mat4.createFromColumns(
            vec4(1,2,3,4), vec4(5,6,7,8), vec4(9,10,11,12), vec4(13,14,15,16)
        );
        Mat4.print('[Mat4.print (col major)]', m);
        Mat4.print('[Mat4.print (row major)]', m, true);

        Uuid.print('[Uuid.print]', Uuid.fromString(Uuid.toString(0)));
    }
}
