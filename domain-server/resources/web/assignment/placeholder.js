/* Add your JavaScript for assignment below this line */

// here are some examples of things you can call
Avatar.position = {x: 0, y: 0.565925, z: 10};
Avatar.chatMessage = "I am not a robot!";
Avatar.handPosition = {x: 0, y: 4.5, z: 0};

// here I'm creating a function to fire before each data send
function dance() {
  // switch the body yaw from 1 to 90
  var randomAngle = Math.floor(Math.random() * 90);
  if (Math.random() < 0.5) {
    randomAngle * -1;
  }
  
  Avatar.bodyYaw = randomAngle;
}

// register the call back so it fires before each data send
Agent.preSendCallback.connect(dance);