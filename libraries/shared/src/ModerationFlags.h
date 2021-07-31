//
//  ModerationFlags.h
//  libraries/shared/src
//
//  Created by Kalila L. on Mar 11 2021.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef vircadia_ModerationFlags_h
#define vircadia_ModerationFlags_h

class ModerationFlags {
public:
    
    /*@jsdoc
     * <p>A set of flags for moderation ban actions. The value is constructed by using the <code>|</code> (bitwise OR) operator on the 
     * individual flag values.</p>
     * <table>
     *   <thead>
     *     <tr><th>Flag Name</th><th>Value</th><th>Description</th></tr>
     *   </thead>
     *   <tbody>
     *     <tr><td>NO_BAN</td><td><code>0</code></td><td>Don't ban user when kicking. <em>This does not currently have an effect.</em></td></tr>
     *     <tr><td>BAN_BY_USERNAME</td><td><code>1</code></td><td>Ban the person by their username.</td></tr>
     *     <tr><td>BAN_BY_FINGERPRINT</td><td><code>2</code></td><td>Ban the person by their machine fingerprint.</td></tr>
     *     <tr><td>BAN_BY_IP</td><td><code>4</code></td><td>Ban the person by their IP address.</td></tr>
     *   </tbody>
     * </table>
     * @typedef {number} BanFlags
     */
    enum BanFlags
    {
        NO_BAN = 0,
        BAN_BY_USERNAME = 1,
        BAN_BY_FINGERPRINT = 2,
        BAN_BY_IP = 4
    };
    
    static constexpr unsigned int getDefaultBanFlags() { return (BanFlags::BAN_BY_USERNAME | BanFlags::BAN_BY_FINGERPRINT); };
};

#endif // vircadia_ModerationFlags_h
