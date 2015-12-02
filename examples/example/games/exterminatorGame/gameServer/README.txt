This gameserver sets up a server with websockets that listen for messages from interface regarding when users shoot rats, and updates a real-time game board with that information. This is just a first pass, and the plan is to abstract this to work with any kind of game content creators wish to make with High Fidelity.

To enter the game: Run pistol.js and shoot at rats.
For every rat you kill, you get a point.
You're score will be displayed at https://desolate-bastion-1742.herokuapp.com/