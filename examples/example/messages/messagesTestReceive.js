var messagesReceivedCount = 0;

function handleMessages(channel, message, sender) {
    print('GOT MESSAGE')
    if (sender === MyAvatar.sessionUUID) {
        if (channel === 'messageTest') {
            messagesReceivedCount++;
            print('sendIndex/receiveCount::' + message + "/" + messagesReceivedCount);
        }

    }
}

Messages.messageReceived.connect(handleMessages);
Messages.subscribe('messageTest')
print('READY TO RECEIVE')