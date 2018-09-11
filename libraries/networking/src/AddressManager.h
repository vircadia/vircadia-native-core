//
//  AddressManager.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2014-09-10.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AddressManager_h
#define hifi_AddressManager_h

#include <QtCore/QObject>
#include <QtCore/QStack>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <DependencyManager.h>

#include "AccountManager.h"

extern const QString DEFAULT_HIFI_ADDRESS;
extern const QString REDIRECT_HIFI_ADDRESS;

const QString SANDBOX_HIFI_ADDRESS = "hifi://localhost";
const QString INDEX_PATH = "/";

const QString GET_PLACE = "/api/v1/places/%1";

/**jsdoc
 * The location API provides facilities related to your current location in the metaverse.
 *
 * <h5>Getter/Setter</h5>
 * <p>You can get and set your current metaverse address by directly reading a string value from and writing a string value to 
 * the <code>location</code> object. This is an alternative to using the <code>location.href</code> property or this object's
 * functions.</p>
 *
 * @namespace location
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-assignment-client
 *
 * @property {Uuid} domainID - A UUID uniquely identifying the domain you're visiting. Is {@link Uuid|Uuid.NULL} if you're not
 *     connected to the domain or are in a serverless domain.
 *     <em>Read-only.</em>
 * @property {string} hostname - The name of the domain for your current metaverse address (e.g., <code>"AvatarIsland"</code>,
 *     <code>localhost</code>, or an IP address). Is blank if you're in a serverless domain.
 *     <em>Read-only.</em>
 * @property {string} href - Your current metaverse address (e.g., <code>"hifi://avatarisland/15,-10,26/0,0,0,1"</code>)
 *     regardless of whether or not you're connected to the domain. Starts with <code>"file:///"</code> if you're in a 
 *     serverless domain.
 *     <em>Read-only.</em>
 * @property {boolean} isConnected - <code>true</code> if you're connected to the domain in your current <code>href</code>
 *     metaverse address, otherwise <code>false</code>.
 * @property {string} pathname - The location and orientation in your current <code>href</code> metaverse address 
 *     (e.g., <code>"/15,-10,26/0,0,0,1"</code>).
 *     <em>Read-only.</em>
 * @property {string} placename - The place name in your current <code>href</code> metaverse address
 *     (e.g., <code>"AvatarIsland"</code>). Is blank if your <code>hostname</code> is an IP address.
 *     <em>Read-only.</em>
 * @property {string} protocol - The protocol of your current <code>href</code> metaverse address (e.g., <code>"hifi"</code>).
 *     <em>Read-only.</em>
 */

class AddressManager : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    Q_PROPERTY(bool isConnected READ isConnected)
    Q_PROPERTY(QUrl href READ currentShareableAddress)
    Q_PROPERTY(QString protocol READ getProtocol)
    Q_PROPERTY(QString hostname READ getHost)
    Q_PROPERTY(QString pathname READ currentPath)
    Q_PROPERTY(QString placename READ getPlaceName)
    Q_PROPERTY(QString domainID READ getDomainID)
public:
    using PositionGetter = std::function<glm::vec3()>;
    using OrientationGetter = std::function<glm::quat()>;

    /**jsdoc
     * <p>The reasons for an address lookup via the metaverse API are defined by numeric values:</p>
     * <table>
     *   <thead>
     *     <tr>
     *       <th>Name</th>
     *       <th>Value</th>
     *       <th>Description</th>
     *     </tr>
     *   </thead>
     *   <tbody>
     *     <tr>
     *       <td><strong>UserInput</strong></td>
     *       <td><code>0</code></td>
     *       <td>User-typed input.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>Back</strong></td>
     *       <td><code>1</code></td>
     *       <td>Address from a {@link location.goBack|goBack} call.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>Forward</strong></td>
     *       <td><code>2</code></td>
     *       <td>Address from a {@link location.goForward|goForward} call.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>StartupFromSettings</strong></td>
     *       <td><code>3</code></td>
     *       <td>Initial location at Interface start-up from settings.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>DomainPathResponse</strong></td>
     *       <td><code>4</code></td>
     *       <td>A named path in the domain.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>Internal</strong></td>
     *       <td><code>5</code></td>
     *       <td>An internal attempt to resolve an alternative path.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>AttemptedRefresh</strong></td>
     *       <td><code>6</code></td>
     *       <td>A refresh after connecting to a domain.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>Suggestions</strong></td>
     *       <td><code>7</code></td>
     *       <td>Address from the Goto dialog.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>VisitUserFromPAL</strong></td>
     *       <td><code>8</code></td>
     *       <td>User from the People dialog.</td>
     *     </tr>
     *   </tbody>
     * </table>
     * @typedef {number} location.LookupTrigger
     */
    enum LookupTrigger
    {
        UserInput,
        Back,
        Forward,
        StartupFromSettings,
        DomainPathResponse,
        Internal,
        AttemptedRefresh,
        Suggestions,
        VisitUserFromPAL
    };

    bool isConnected();
    QString getProtocol() const;

    QUrl currentAddress(bool domainOnly = false) const;
    QUrl currentFacingAddress() const;
    QUrl currentShareableAddress(bool domainOnly = false) const;
    QUrl currentPublicAddress(bool domainOnly = false) const;
    QUrl currentFacingShareableAddress() const;
    QUrl currentFacingPublicAddress() const;
    QString currentPath(bool withOrientation = true) const;
    QString currentFacingPath() const;

    QUrl lastAddress() const;

    const QUuid& getRootPlaceID() const { return _rootPlaceID; }
    QString getPlaceName() const;
    QString getDomainID() const;

    QString getHost() const { return _domainURL.host(); }

    void setPositionGetter(PositionGetter positionGetter) { _positionGetter = positionGetter; }
    void setOrientationGetter(OrientationGetter orientationGetter) { _orientationGetter = orientationGetter; }

    void loadSettings(const QString& lookupString = QString());

    const QStack<QUrl>& getBackStack() const { return _backStack; }
    const QStack<QUrl>& getForwardStack() const { return _forwardStack; }

    QUrl getDomainURL() { return _domainURL; }

public slots:
    /**jsdoc
     * Go to a specified metaverse address.
     * @function location.handleLookupString
     * @param {string} address - The address to go to: a <code>"hifi://"<code> address, an IP address (e.g., 
     * <code>"127.0.0.1"</code> or <code>"localhost"</code>), a domain name, a named path on a domain (starts with 
     * <code>"/"</code>), a position or position and orientation, or a user (starts with <code>"@"</code>).
     * @param {boolean} fromSuggestions=false - Set to <code>true</code> if the address is obtained from the "Goto" dialog.
     *    Helps ensure that user's location history is correctly maintained.
     */
    void handleLookupString(const QString& lookupString, bool fromSuggestions = false);

    /**jsdoc
     * Go to a position and orientation resulting from a lookup for a named path in the domain (set in the domain server's 
     * settings).
     * @function location.goToViewpointForPath
     * @param {string} path - The position and orientation corresponding to the named path.
     * @param {string} namedPath - The named path that was looked up on the server.
     * @deprecated This function is deprecated and will be removed.
     */
    // This function is marked as deprecated in anticipation that it will not be included in the JavaScript API if and when the
    // functions and signals that should be exposed are moved to a scripting interface class.
    //
    // we currently expect this to be called from NodeList once handleLookupString has been called with a path
    bool goToViewpointForPath(const QString& viewpointString, const QString& pathString) {
        return handleViewpoint(viewpointString, false, DomainPathResponse, false, pathString);
    }

    /**jsdoc
     * Go back to the previous location in your navigation history, if there is one.
     * @function location.goBack
     */
    void goBack();

    /**jsdoc
     * Go forward to the next location in your navigation history, if there is one.
     * @function location.goForward
     */
    void goForward();

    /**jsdoc
     * Go to the local Sandbox server that's running on the same PC as Interface.
     * @function location.goToLocalSandbox
     * @param {string} path="" - The position and orientation to go to (e.g., <code>"/0,0,0"</code>).
     * @param {location.LookupTrigger} trigger=StartupFromSettings - The reason for the function call. Helps ensure that user's
     *     location history is correctly maintained.
     */
    void goToLocalSandbox(QString path = "", LookupTrigger trigger = LookupTrigger::StartupFromSettings) {
        handleUrl(SANDBOX_HIFI_ADDRESS + path, trigger);
    }

    /**jsdoc
     * Go to the default "welcome" metaverse address.
     * @function location.goToEntry
     * @param {location.LookupTrigger} trigger=StartupFromSettings - The reason for the function call. Helps ensure that user's
     *     location history is correctly maintained.
     */
    void goToEntry(LookupTrigger trigger = LookupTrigger::StartupFromSettings) { handleUrl(DEFAULT_HIFI_ADDRESS, trigger); }

    /**jsdoc
     * Go to the specified user's location.
     * @function location.goToUser
     * @param {string} username - The user's username.
     * @param {boolean} matchOrientation=true - If <code>true</code> then go to a location just in front of the user and turn to face
     *     them, otherwise go to the user's exact location and orientation.
     */
    void goToUser(const QString& username, bool shouldMatchOrientation = true);

    /**jsdoc
    * Go to the last address tried.  This will be the last URL tried from location.handleLookupString
    * @function location.goToLastAddress
    */
    void goToLastAddress() { handleUrl(_lastVisitedURL, LookupTrigger::AttemptedRefresh); }

    /**jsdoc
     * Refresh the current address, e.g., after connecting to a domain in order to position the user to the desired location.
     * @function location.refreshPreviousLookup
     * @deprecated This function is deprecated and will be removed.
     */
    // This function is marked as deprecated in anticipation that it will not be included in the JavaScript API if and when the
    // functions and signals that should be exposed are moved to a scripting interface class.
    void refreshPreviousLookup();

    /**jsdoc
     * Update your current metaverse location in Interface's {@link Settings} file as your last-known address. This can be used
     * to ensure that you start up at that address if you exit Interface without a later address automatically being saved.
     * @function location.storeCurrentAddress
     */
    void storeCurrentAddress();

    /**jsdoc
     * Copy your current metaverse address (i.e., <code>location.href</code> property value) to the OS clipboard.
     * @function location.copyAddress
     */
    void copyAddress();

    /**jsdoc
     * Copy your current metaverse location and orientation (i.e., <code>location.pathname</code> property value) to the OS 
     * clipboard.
     * @function location.copyPath
     */
    void copyPath();

    /**jsdoc
     * Retrieve and remember the place name for the given domain ID if the place name is not already known.
     * @function location.lookupShareableNameForDomainID
     * @param {Uuid} domainID - The UUID of the domain.
     * @deprecated This function is deprecated and will be removed.
     */
    // This function is marked as deprecated in anticipation that it will not be included in the JavaScript API if and when the
    // functions and signals that should be exposed are moved to a scripting interface class.
    void lookupShareableNameForDomainID(const QUuid& domainID);

signals:
    /**jsdoc
     * Triggered when looking up the details of a metaverse user or location to go to has completed (successfully or
     * unsuccessfully).
     * @function location.lookupResultsFinished
     * @returns {Signal}
     */
    void lookupResultsFinished();

    /**jsdoc
     * Triggered when looking up the details of a metaverse user or location to go to has completed and the domain or user is 
     * offline.
     * @function location.lookupResultIsOffline
     * @returns {Signal}
     */
    void lookupResultIsOffline();

    /**jsdoc
     * Triggered when looking up the details of a metaverse user or location to go to has completed and the domain or user could
     * not be found.
     * @function location.lookupResultIsNotFound
     * @returns {Signal}
     */
    void lookupResultIsNotFound();

    /**jsdoc
     * Triggered when a request is made to go to an IP address.
     * @function location.possibleDomainChangeRequired
     * @param {Url} domainURL - URL for domain
     * @param {Uuid} domainID - The UUID of the domain to go to.
     * @returns {Signal}
     */
    // No example because this function isn't typically used in scripts.
    void possibleDomainChangeRequired(QUrl domainURL, QUuid domainID);

    /**jsdoc
     * Triggered when a request is made to go to a named domain or user.
     * @function location.possibleDomainChangeRequiredViaICEForID
     * @param {string} iceServerHostName - IP address of the ICE server.
     * @param {Uuid} domainID - The UUID of the domain to go to.
     * @returns {Signal}
     */
    // No example because this function isn't typically used in scripts.
    void possibleDomainChangeRequiredViaICEForID(const QString& iceServerHostname, const QUuid& domainID);

    /**jsdoc
     * Triggered when an attempt is made to send your avatar to a specified position on the current domain. For example, when
     * you change domains or enter a position to go to in the "Goto" dialog.
     * @function location.locationChangeRequired
     * @param {Vec3} position - The position to go to.
     * @param {boolean} hasOrientationChange - If <code>true</code> then a new <code>orientation</code> has been requested.
     * @param {Quat} orientation - The orientation to change to. Is {@link Quat(0)|Quat.IDENTITY} if 
     *     <code>hasOrientationChange</code> is <code>false</code>.
     * @param {boolean} shouldFaceLocation - If <code>true</code> then the request is to go to a position near that specified 
     *     and orient your avatar to face it. For example when you visit someone from the "People" dialog.
     * @returns {Signal}
     * @example <caption>Report location change requests.</caption>
     * function onLocationChangeRequired(newPosition, hasOrientationChange, newOrientation, shouldFaceLocation) {
     *     print("Location change required:");
     *     print("- New position = " + JSON.stringify(newPosition));
     *     print("- Has orientation change = " + hasOrientationChange);
     *     print("- New orientation = " + JSON.stringify(newOrientation));
     *     print("- Should face location = " + shouldFaceLocation);
     * }
     *
     * location.locationChangeRequired.connect(onLocationChangeRequired);
     */
    void locationChangeRequired(const glm::vec3& newPosition,
                                bool hasOrientationChange,
                                const glm::quat& newOrientation,
                                bool shouldFaceLocation);

    /**jsdoc
     * Triggered when an attempt is made to send your avatar to a new named path on the domain (set in the domain server's
     * settings). For example, when you enter a "/" followed by the path's name in the "GOTO" dialog.
     * @function location.pathChangeRequired
     * @param {string} path - The name of the path to go to.
     * @returns {Signal}
     * @example <caption>Report path change requests.</caption>
     * function onPathChangeRequired(newPath) {
     *     print("onPathChangeRequired: newPath = " + newPath);
     * }
     *
     * location.pathChangeRequired.connect(onPathChangeRequired);
     */
    void pathChangeRequired(const QString& newPath);

    /**jsdoc
     * Triggered when you navigate to a new domain.
     * @function location.hostChanged
     * @param {string} hostname - The new domain's host name.
     * @returns {Signal}
     * @example <caption>Report when you navigate to a new domain.</caption>
     * function onHostChanged(host) {
     *     print("Host changed to: " + host);
     * }
     *
     * location.hostChanged.connect(onHostChanged);
     */
    void hostChanged(const QString& newHost);

    /**jsdoc
     * Triggered when there's a change in whether or not there's a previous location that can be navigated to using
     * {@link location.goBack|goBack}. (Reflects changes in the state of the "Goto" dialog's back arrow.)
     * @function location.goBackPossible
     * @param {boolean} isPossible - <code>true</code> if there's a previous location to navigate to, otherwise 
     *     <code>false</code>.
     * @returns {Signal}
     * @example <caption>Report when ability to navigate back changes.</caption>
     * function onGoBackPossible(isPossible) {
     *     print("Go back possible: " + isPossible);
     * }
     *
     * location.goBackPossible.connect(onGoBackPossible);
     */
    void goBackPossible(bool isPossible);

    /**jsdoc
     * Triggered when there's a change in whether or not there's a forward location that can be navigated to using
     * {@link location.goForward|goForward}. (Reflects changes in the state of the "Goto" dialog's forward arrow.)
     * @function location.goForwardPossible
     * @param {boolean} isPossible - <code>true</code> if there's a forward location to navigate to, otherwise
     *     <code>false</code>.
     * @returns {Signal}
     * @example <caption>Report when ability to navigate forward changes.</caption>
     * function onGoForwardPossible(isPossible) {
     *     print("Go forward possible: " + isPossible);
     * }
     *
     * location.goForwardPossible.connect(onGoForwardPossible);
     */
    void goForwardPossible(bool isPossible);

private slots:
    void handleAPIResponse(QNetworkReply* requestReply);
    void handleAPIError(QNetworkReply* errorReply);

    void handleShareableNameAPIResponse(QNetworkReply* requestReply);

private:
    void goToAddressFromObject(const QVariantMap& addressMap, const QNetworkReply* reply);

    // Set host and port, and return `true` if it was changed.
    bool setHost(const QString& host, LookupTrigger trigger, quint16 port = 0);
    bool setDomainInfo(const QUrl& domainURL, LookupTrigger trigger);

    const JSONCallbackParameters& apiCallbackParameters();

    bool handleUrl(const QUrl& lookupUrl, LookupTrigger trigger = UserInput);

    bool handleNetworkAddress(const QString& lookupString, LookupTrigger trigger, bool& hostChanged);
    void handlePath(const QString& path, LookupTrigger trigger, bool wasPathOnly = false);
    bool handleViewpoint(const QString& viewpointString,
                         bool shouldFace,
                         LookupTrigger trigger,
                         bool definitelyPathOnly = false,
                         const QString& pathString = QString());
    bool handleUsername(const QString& lookupString);
    bool handleDomainID(const QString& host);

    void attemptPlaceNameLookup(const QString& lookupString, const QString& overridePath, LookupTrigger trigger);
    void attemptDomainIDLookup(const QString& lookupString, const QString& overridePath, LookupTrigger trigger);

    void addCurrentAddressToHistory(LookupTrigger trigger);

    QUrl _domainURL;
    QUrl _lastVisitedURL;

    QUuid _rootPlaceID;
    PositionGetter _positionGetter;
    OrientationGetter _orientationGetter;

    QString _shareablePlaceName;

    QStack<QUrl> _backStack;
    QStack<QUrl> _forwardStack;
    quint64 _lastBackPush = 0;

    QString _newHostLookupPath;

    QUrl _previousAPILookup;
};

#endif  // hifi_AddressManager_h
