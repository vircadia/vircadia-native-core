var messageIndex = 1;

var messageRate = 500;
function sendMessage(){
    print('SENDING MESSAGE')
Messages.sendMessage('messageTest',messageIndex)
messageIndex++;
}

function cleanup(){
    Script.clearInterval(messageInterval);
}

var messageInterval = Script.setInterval(function(){
    sendMessage();
},messageRate);

Script.scriptEnding.connect(cleanup);