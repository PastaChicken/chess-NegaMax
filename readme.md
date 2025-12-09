
This contains my inital writing of chess & implemntation of AI for chess
Add support for FEN stringsLinks to an external site. to your game setup so that instead of the current way you are setting up your game board you are setting it up with a call similar to the following call.

FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

Your routine should be able to take just the board position portion of a FEN string, or the entire FEN string like so:

FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

Bugs known: Pawn piece I was unable to fix  the player being able to move their pawn on any square in the first two rows when first playing a pawn piece (does not affect ai and  only visually shows what legal moves are possible).

UPDATE:

Behind on implementing rest of chess pieces but was able to get everything working and also implemented alpha beta pruning for ai but will still implement a better evaluation function for ai as AI rn just goes for the king with highest points. (I fortunately (kinda) have to understand how to play chess of at least a playstyle for the chest to go for in evaluate board?)