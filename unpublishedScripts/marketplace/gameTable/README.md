Game Table Documentation
=============

Ready to play?  Try out our new gaming table, with five games to choose from:  checkers, chess, dominoes, dice, and a deck of cards!  Add your own game!

The gaming table is designed with the following principles:

1. Gets its list of games from a remote location, and so can be updated without requiring a new download by users.
2. New games can be defined in a simple JSON data structure.
3. Games are made for using hand controllers.  Pieces are physical as much as possible.
4. Simple controls:  reset game, next game.
5. No 'rules' are enforced.  It's up to players to respect each other.  


ADDING GAMES
============

There are two spawning styles for games, depending on the type of game you're making:  'arranged' spawning and 'pile' spawning.

For 'arranged' style games, the table divides its surface into tiles based on how many rows are in the arrangement.   Currently, the boards must be square (i.e. 8x8 or 10x10).  Then it goes through the starting arrangement and pastes the JSON for each piece at the appropriate location. A player number in front of the piece name in the game definition helps pick the right color piece for games where player pieces are similar.  Releasing a held piece with 'snapToGrid' on will result in that piece snapping to the nearest available tile.

JSON GAME PARAMETERS
=============
(BOTH STYLES)
- @gameName - string - what the game is called.```checkers```
- @matURL - string - url of picture to put on the table.  ```http://mywebsite/checkers.jpg```
- @spawnStyle - string - either ```pile``` or ```arranged```.

(ARRANGED STYLE)
- @snapToGrid - boolean - should pieces snap to tiles
- @startingArrangement - 2d array - ['playerNumber:pieceName'] -  ```["1:rook","2:queen"]```
- @pieces - array of object mappings - one object mapping per player -{pieceName:JSONurl} - ```[{"king": "http://mywebsite/chess/king_black.json"},{"king": "http://mywebsite/chess/king_white.json"}]```

(PILE STYLE)
- @identicalPieces - boolean - whether or not the pieces are identical.  if ```true```, only the first piece is used and repeatedly spawned.
- @howMany - number - the amount of pieces to creates
- @pieces - array of strings - ```["http://mywebsite/domino.json"]```


TESTING
=============
Change the GAMES_LIST_ENDPOINT variable in table.js to point at your array of game .json files on a local or remote server.

The list should look more or less like this: https://api.myjson.com/bins/437s6

PUBLISHING
=============
- As a script: just add createGameTable.js to the marketplace.

- As an item: run createGameTable.js and select everything, export it to json.  Go through the usual marketplace uploading and prep sequence.
