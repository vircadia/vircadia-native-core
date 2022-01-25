//
//  MessagesClient.h
//  libraries/networking/src
//
//  Created by Brad hefta-Gaub on 11/16/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_MessagesClient_h
#define hifi_MessagesClient_h

#include <QString>
#include <QByteArray>
#include <QtCore/QSharedPointer>

#include <DependencyManager.h>

#include "LimitedNodeList.h"
#include "NLPacket.h"
#include "Node.h"
#include "ReceivedMessage.h"

/*@jsdoc
 * <p>The <code>Messages</code> API enables text and data to be sent between scripts over named "channels". A channel can have 
 * an arbitrary name to help separate messaging between different sets of scripts.</p>
 *
 * <p><strong>Note:</strong> To call a function in another script, you should use one of the following rather than sending a 
 * message:</p>
 * <ul>
 *   <li>{@link Entities.callEntityClientMethod}</li>
 *   <li>{@link Entities.callEntityMethod}</li>
 *   <li>{@link Entities.callEntityServerMethod}</li>
 *   <li>{@link Script.callEntityScriptMethod}</li>
 * </ul>
 *
 * @namespace Messages
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 * @hifi-server-entity
 * @hifi-assignment-client
 */
class MessagesClient : public QObject, public Dependency {
    Q_OBJECT
public:
    MessagesClient();
    
    void startThread();

    /*@jsdoc
     * Sends a text message on a channel.
     * @function Messages.sendMessage
     * @param {string} channel - The channel to send the message on.
     * @param {string} message - The message to send.
     * @param {boolean} [localOnly=false] - If <code>false</code> then the message is sent to all Interface, client entity, 
     *     server entity, and assignment client scripts in the domain.
     *     <p>If <code>true</code> then: if sent from an Interface or client entity script it is received by all Interface and 
     *     client entity scripts; if sent from a server entity script it is received by all entity server scripts; and if sent 
     *     from an assignment client script it is received only by that same assignment client script.</p>
     * @example <caption>Send and receive a message.</caption>
     * // Receiving script.
     * var channelName = "com.vircadia.example.messages-example";
     *
     * function onMessageReceived(channel, message, sender, localOnly) {
     *     print("Message received:");
     *     print("- channel: " + channel);
     *     print("- message: " + message);
     *     print("- sender: " + sender);
     *     print("- localOnly: " + localOnly);
     * }
     *
     * Messages.subscribe(channelName);
     * Messages.messageReceived.connect(onMessageReceived);
     *
     * Script.scriptEnding.connect(function () {
     *     Messages.messageReceived.disconnect(onMessageReceived);
     *     Messages.unsubscribe(channelName);
     * });
     *
     *
     * // Sending script.
     * var channelName = "com.vircadia.example.messages-example";
     * var message = "Hello";
     * Messages.sendMessage(channelName, message);
     */
    Q_INVOKABLE void sendMessage(QString channel, QString message, bool localOnly = false);

    /*@jsdoc
     * Sends a text message locally on a channel.
     * This is the same as calling {@link Messages.sendMessage|sendMessage} with <code>localOnly == true</code>.
     * @function Messages.sendLocalMessage
     * @param {string} channel - The channel to send the message on.
     * @param {string} message - The message to send.
     */
    Q_INVOKABLE void sendLocalMessage(QString channel, QString message);

    /*@jsdoc
     * Sends a data message on a channel.
     * @function Messages.sendData
     * @param {string} channel - The channel to send the data on.
     * @param {object} data - The data to send. The data is handled as a byte stream, for example, as may be provided via a 
     *     JavaScript <code>Int8Array</code> object.
     * @param {boolean} [localOnly=false] - If <code>false</code> then the message is sent to all Interface, client entity,
     *     server entity, and assignment client scripts in the domain.
     *     <p>If <code>true</code> then: if sent from an Interface or client entity script it is received by all Interface and
     *     client entity scripts; if sent from a server entity script it is received by all entity server scripts; and if sent
     *     from an assignment client script it is received only by that same assignment client script.</p>
     * @example <caption>Send and receive data.</caption>
     * // Receiving script.
     * var channelName = "com.vircadia.example.messages-example";
     *
     * function onDataReceived(channel, data, sender, localOnly) {
     *     var int8data = new Int8Array(data);
     *     var dataAsString = "";
     *     for (var i = 0; i < int8data.length; i++) {
     *         if (i > 0) {
     *             dataAsString += ", ";
     *         }
     *         dataAsString += int8data[i];
     *     }
     *     print("Data received:");
     *     print("- channel: " + channel);
     *     print("- data: " + dataAsString);
     *     print("- sender: " + sender);
     *     print("- localOnly: " + localOnly);
     * }
     *
     * Messages.subscribe(channelName);
     * Messages.dataReceived.connect(onDataReceived);
     *
     * Script.scriptEnding.connect(function () {
     *     Messages.dataReceived.disconnect(onDataReceived);
     *     Messages.unsubscribe(channelName);
     * });
     *
     *
     * // Sending script.
     * var channelName = "com.vircadia.example.messages-example";
     * var int8data = new Int8Array([1, 1, 2, 3, 5, 8, 13]);
     * Messages.sendData(channelName, int8data.buffer);
     */
    Q_INVOKABLE void sendData(QString channel, QByteArray data, bool localOnly = false);
    
    /*@jsdoc
     * Subscribes the scripting environment &mdash; Interface, the entity script server, or assignment client instance &mdash; 
     * to receive messages on a specific channel. This means, for example, that if there are two Interface scripts that 
     * subscribe to different channels, both scripts will receive messages on both channels.
     * @function Messages.subscribe
     * @param {string} channel - The channel to subscribe to.
     */
    Q_INVOKABLE void subscribe(QString channel);

    /*@jsdoc
     * Unsubscribes the scripting environment from receiving messages on a specific channel.
     * @function Messages.unsubscribe
     * @param {string} channel - The channel to unsubscribe from.
     */
    Q_INVOKABLE void unsubscribe(QString channel);

    static void decodeMessagesPacket(QSharedPointer<ReceivedMessage> receivedMessage, QString& channel, 
                                           bool& isText, QString& message, QByteArray& data, QUuid& senderID);

    static std::unique_ptr<NLPacketList> encodeMessagesPacket(QString channel, QString message, QUuid senderID);
    static std::unique_ptr<NLPacketList> encodeMessagesDataPacket(QString channel, QByteArray data, QUuid senderID);

signals:
    /*@jsdoc
     * Triggered when a text message is received.
     * @function Messages.messageReceived
     * @param {string} channel - The channel that the message was sent on. This can be used to filter out messages not relevant 
     *     to your script.
     * @param {string} message - The message received.
     * @param {Uuid} senderID - The UUID of the sender: the user's session UUID if sent by an Interface or client entity 
     *     script, the UUID of the entity script server if sent by a server entity script, or the UUID of the assignment client 
     *     instance if sent by an assignment client script.
     * @param {boolean} localOnly - <code>true</code> if the message was sent with <code>localOnly == true</code>.
     * @returns {Signal}
     */
    void messageReceived(QString channel, QString message, QUuid senderUUID, bool localOnly);

    /*@jsdoc
     * Triggered when a data message is received.
     * @function Messages.dataReceived
     * @param {string} channel - The channel that the message was sent on. This can be used to filter out messages not relevant
     *     to your script.
     * @param {object} data - The data received. The data is handled as a byte stream, for example, as may be used by a 
     *     JavaScript <code>Int8Array</code> object.
     * @param {Uuid} senderID - The UUID of the sender: the user's session UUID if sent by an Interface or client entity
     *     script, the UUID of the entity script server if sent by a server entity script, or the UUID of the assignment client
     *     instance if sent by an assignment client script.
     * @param {boolean} localOnly - <code>true</code> if the message was sent with <code>localOnly == true</code>.
     * @returns {Signal}
     */
    void dataReceived(QString channel, QByteArray data, QUuid senderUUID, bool localOnly);

private slots:
    void handleMessagesPacket(QSharedPointer<ReceivedMessage> receivedMessage, SharedNodePointer senderNode);
    void handleNodeActivated(SharedNodePointer node);

protected:
    QSet<QString> _subscribedChannels;
};

#endif
