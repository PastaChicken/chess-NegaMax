#include "Chess.h"
#include "MagicBitboards.h"
#include <limits>
#include <cmath>
#include "BitHolder.h"

Chess::Chess()
{
    _grid = new Grid(8, 8);
    // Precompute knight move bitboards for each square
    
    initMagicBitboards();

    for(int i = 0; i < 128; i++) { _bitBoardLookup[i] = 0; }
    // Map characters from pieceNotation / FEN to our bitboard indices
    _bitBoardLookup['0'] = EMPTY_SQUARES;
    _bitBoardLookup['P'] = WHITE_PAWNS;
    _bitBoardLookup['N'] = WHITE_KNIGHTS;
    _bitBoardLookup['B'] = WHITE_BISHOPS;
    _bitBoardLookup['R'] = WHITE_ROOKS;
    _bitBoardLookup['Q'] = WHITE_QUEENS;
    _bitBoardLookup['K'] = WHITE_KING;
    _bitBoardLookup['p'] = BLACK_PAWNS;
    _bitBoardLookup['n'] = BLACK_KNIGHTS;
    _bitBoardLookup['b'] = BLACK_BISHOPS;
    _bitBoardLookup['r'] = BLACK_ROOKS;
    _bitBoardLookup['q'] = BLACK_QUEENS;
    _bitBoardLookup['k'] = BLACK_KING;

    
}

Chess::~Chess()
{
    cleanupMagicBitboards();
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
    _currentPlayer = WHITE;
    _moves = generateAllMoves(stateString(), _currentPlayer);

    if (gameHasAI()) {
        setAIPlayer(AI_PLAYER);
    }

    startGame();
}

void Chess::FENtoBoard(const std::string& fen) {
    // convert a FEN string to a board
    // FEN is a space delimited string with 6 fields
    // 1: piece placement (from white's perspective)
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->setBit(nullptr);
    });

    int row = 7;
    int col = 0;

    for (char ch: fen) {
        if(ch == '/') {
            row--;
            col = 0;
        } else if (isdigit(ch)) {
            col += ch - '0';
        } else {
            ChessPiece piece;
            int playerNumber;

            switch (toupper(ch)) {
                case 'P':
                    piece = Pawn;
                    break;
                case 'N':
                    piece = Knight;
                    break;
                case 'B':
                    piece = Bishop;
                    break;
                case 'R':
                    piece = Rook;
                    break;
                case 'Q':
                    piece = Queen;
                    break;
                case 'K':
                    piece = King;
                    break;
                default:
                    piece = Pawn; // default to pawn for safety maybe? Not needed if string is always valid
            }
            Bit *bit = PieceForPlayer(isupper(ch) ? 0 : 1, piece);
            ChessSquare* square = _grid->getSquare(col, row);
            bit->setPosition(square->getPosition());
            bit->setParent(square);
            bit->setGameTag(isupper(ch) ? piece : piece + 128);
            square->setBit(bit);
            col++;
        }
    }

    // NOT PART OF THIS ASSIGNMENT BUT OTHER THINGS THAT CAN BE IN A FEN STRING
    // ARE BELOW
    // 2: active color (W or B)
    // 3: castling availability (KQkq or -)
    // 4: en passant target square (in algebraic notation, or -)
    // 5: halfmove clock (number of halfmoves since the last capture or pawn advance)
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (currentPlayer != pieceColor) {
        return false;
    }
    bool ret = false;
    ChessSquare* sourceSquare = (ChessSquare *)&src;
    // Clear any previous highlights, then show legal destinations for this piece
    clearBoardHighlights();
    if (sourceSquare) {
        int squareIndex = sourceSquare->getSquareIndex();
        for (auto &move : _moves) {
            if (move.from == squareIndex) {
                ret = true;
                auto dest = _grid->getSquareByIndex(move.to);
                if (dest) dest->setHighlighted(true);
            }
        }
    }

    return ret;
   
}
//implement movement for each piece type starting with pawns, knights, and king
//and possibly show legal moves when a piece is selected
bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    
    ChessSquare* destSquare = (ChessSquare *)&dst;
    if(destSquare) {
        int squareIndex = destSquare->getSquareIndex();
        for(auto move : _moves) {
            if(move.to == squareIndex) {
                return true;
            }
        }
    }
    return false;
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    // After a successful move, switch players and generate new move list
    clearBoardHighlights();
    _currentPlayer = (_currentPlayer == WHITE) ? BLACK : WHITE;
    _moves = generateAllMoves(stateString(), _currentPlayer);
    endTurn();
}




void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}
void Chess::addPawnBitboardMovesToList(std::vector<BitMove>& moves, const BitBoard bitboard, const int direction)
{
    if(bitboard.getData() == 0) return;
    bitboard.forEachBit([&](int fromSquare) {
        int toSquare = toSquare - 2;
        moves.emplace_back(BitMove(fromSquare, toSquare, Pawn));
    });
}
void Chess::generatePawnMoveList(std::vector<BitMove>& moves, const BitBoard pawns, const BitBoard emptySquares, const BitBoard enemyPieces, char color)
{
   if (pawns.getData() == 0) return;

    uint64_t pawnsData = pawns.getData();
    uint64_t emptyData = emptySquares.getData();
    uint64_t enemyData = enemyPieces.getData();
    // Single forward moves
    uint64_t singleMovesData = (color == WHITE) ? ((pawnsData << 8) & emptyData) : ((pawnsData >> 8) & emptyData);
    BitBoard singleMoves(singleMovesData);
    singleMoves.forEachBit([&](int toSquare) {
        int fromSquare = (color == WHITE) ? (toSquare - 8) : (toSquare + 8);
        moves.emplace_back(fromSquare, toSquare, Pawn);
    });
    // Double forward moves from starting rank
    uint64_t doubleMovesData = (color == WHITE) ? (((singleMovesData & Rank3) << 8) & emptyData) : (((singleMovesData & Rank6) >> 8) & emptyData);
    BitBoard doubleMoves(doubleMovesData);
    doubleMoves.forEachBit([&](int toSquare) {
        int fromSquare = (color == WHITE) ? (toSquare - 16) : (toSquare + 16);
        moves.emplace_back(fromSquare, toSquare, Pawn);
    });

    // Captures
    uint64_t capturesLeftData = (color == WHITE) ? (((pawnsData & NotAFile) << 7) & enemyData) : (((pawnsData & NotAFile) >> 9) & enemyData);
    uint64_t capturesRightData = (color == WHITE) ? (((pawnsData & NotHFile) << 9) & enemyData) : (((pawnsData & NotHFile) >> 7) & enemyData);
    BitBoard capturesLeft(capturesLeftData);
    BitBoard capturesRight(capturesRightData);

    capturesLeft.forEachBit([&](int toSquare) {
        int fromSquare = (color == WHITE) ? (toSquare - 7) : (toSquare + 9);
        moves.emplace_back(fromSquare, toSquare, Pawn);
    });
    capturesRight.forEachBit([&](int toSquare) {
        int fromSquare = (color == WHITE) ? (toSquare - 9) : (toSquare + 7);
        moves.emplace_back(fromSquare, toSquare, Pawn);
    });

    
}

BitBoard Chess::generateKnightMoveBitboard(int square) {
    BitBoard bitboard = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    std::pair<int, int> knightOffsets[] = {
        {2, 1}, {1, 2}, {-1, 2}, {-2, 1},
        {-2, -1}, {-1, -2}, {1, -2}, {2, -1}
    };
    constexpr uint64_t onebit = 1;
    for (auto [dr,df] : knightOffsets) {
        int r = rank + dr, f = file +df;
        if(r >=0 && r < 8 && f >=0 && f < 8) {
            bitboard |= onebit << (r * 8 + f);
            bitboard |= onebit << (r * 8 + f);
        }
    }
    return bitboard;
}



// Generate actual move objects from a bitboard
void Chess::generateKnightMoves(std::vector<BitMove>& moves, BitBoard knightBoard, uint64_t occupancy) {
    knightBoard.forEachBit([&](int fromSquare) {
        BitBoard moveBitboard = BitBoard(KnightAttacks[fromSquare] & occupancy);
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Knight);
        });
    });
}

void Chess::generateKingMoves(std::vector<BitMove>& moves, BitBoard kingBoard, uint64_t occupancy) {
    kingBoard.forEachBit([&](int fromSquare) {
        BitBoard moveBitboard = BitBoard(KingAttacks[fromSquare] & occupancy);
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, King);
        });
    });
}


void Chess::generateBishopMoves(std::vector<BitMove>& moves, BitBoard bishopBoard, uint64_t occupancy, uint64_t friendlies) {
    bishopBoard.forEachBit([&](int fromSquare) {
        BitBoard moveBitboard = BitBoard(getBishopAttacks(fromSquare, occupancy) & ~friendlies);
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Bishop);
        });
    });
}

void Chess::generateRookMoves(std::vector<BitMove>& moves, BitBoard rookBoard, uint64_t occupancy, uint64_t friendlies) {
    rookBoard.forEachBit([&](int fromSquare) {
        BitBoard moveBitboard = BitBoard(getRookAttacks(fromSquare, occupancy) & ~friendlies);
        

        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Rook);
        });
    });
}

void Chess::generateQueenMoves(std::vector<BitMove>& moves, BitBoard queenBoard, uint64_t occupancy, uint64_t friendlies) {
    queenBoard.forEachBit([&](int fromSquare) {
        BitBoard moveBitboard = BitBoard(getQueenAttacks(fromSquare, occupancy) & ~friendlies);

        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Queen);
        });
    });
}




std::vector<BitMove> Chess::generateAllMoves(const std::string& state, int playerColor)
{
   std::vector<BitMove> moves;
    moves.reserve(32);
   
   for (int i = 0; i < eNUM_BITBOARDS; i++) {
        _bitboards[i] = 0;
    }

    for( int i = 0; i < 64; i++ ) {
        int bitIndex = _bitBoardLookup[state[i]];
        _bitboards[bitIndex] |= 1ULL << i;
        if (state[i] != '0') {
            _bitboards[OCCUPANCY] |= 1ULL << i;
            _bitboards[isupper(state[i]) ? WHITE_ALL_PIECEES : BLACK_ALL_PIECES] |= 1ULL << i;
        }
    }
    _bitboards[WHITE_ALL_PIECEES] = _bitboards[WHITE_PAWNS].getData() | _bitboards[WHITE_KNIGHTS].getData() | _bitboards[WHITE_BISHOPS].getData() |
                                    _bitboards[WHITE_ROOKS].getData() | _bitboards[WHITE_QUEENS].getData() | _bitboards[WHITE_KING].getData();

    _bitboards[BLACK_ALL_PIECES] = _bitboards[BLACK_PAWNS].getData() | _bitboards[BLACK_KNIGHTS].getData() | _bitboards[BLACK_BISHOPS].getData() |
                                   _bitboards[BLACK_ROOKS].getData() | _bitboards[BLACK_QUEENS].getData() | _bitboards[BLACK_KING].getData();
    
    _bitboards[OCCUPANCY] = _bitboards[WHITE_ALL_PIECEES].getData() | _bitboards[BLACK_ALL_PIECES].getData();
    int bitIndex = playerColor == WHITE ? WHITE_PAWNS : BLACK_PAWNS;
    // Friendly and enemy aggregate bitboards
    uint64_t occupancyData = _bitboards[OCCUPANCY].getData();
    uint64_t friendlyData = (playerColor == WHITE) ? _bitboards[WHITE_ALL_PIECEES].getData() : _bitboards[BLACK_ALL_PIECES].getData();
    uint64_t enemyData = (playerColor == WHITE) ? _bitboards[BLACK_ALL_PIECES].getData() : _bitboards[WHITE_ALL_PIECEES].getData();
    BitBoard emptySquares(~occupancyData);

    generateKnightMoves(moves, _bitboards[WHITE_KNIGHTS + bitIndex], emptySquares.getData());
    generateKingMoves(moves, _bitboards[WHITE_KING + bitIndex], ~friendlyData);
    generateBishopMoves(moves, _bitboards[WHITE_BISHOPS + bitIndex], occupancyData, friendlyData);
    generatePawnMoveList(moves, _bitboards[WHITE_PAWNS + bitIndex], BitBoard(emptySquares.getData()), BitBoard(enemyData), playerColor);
    generateRookMoves(moves, _bitboards[WHITE_ROOKS + bitIndex], occupancyData, friendlyData);
    generateQueenMoves(moves, _bitboards[WHITE_QUEENS + bitIndex], occupancyData, friendlyData);
    return moves;
}

void Chess::updateAI() {
    int bestVal = negInfite;
    BitMove bestMove;
    std::string state = stateString();

    for(auto move : _moves) {
        int srcSquare = move.from;
        int dstSquare = move.to;
        
        char oldDst = state[dstSquare];
        char srcPece = state[srcSquare];
        state[dstSquare] = state[srcSquare];
        state[dstSquare] = state[srcSquare];
        state[srcSquare] = '0';
        int moveVal = -negamax(state, 5, negInfite, posInfite, HUMAN_PLAYER);
        
        state[dstSquare] = oldDst;
        state[srcSquare] = srcPece;
        if(moveVal > bestVal) {
            bestMove = move;
            bestVal = moveVal;
        }
    };


    // Make the best move
    if(bestVal != negInfite) {
       std::cout << "Moves checked: " << _countMoves << std::endl;

       int srcSquare = bestMove.from;
       int dstSquare = bestMove.to;
       BitHolder& src = getHolderAt(srcSquare&7, srcSquare/8);
        BitHolder& dst = getHolderAt(dstSquare&7, dstSquare/8);
        Bit* bit = src.bit();
        dst.dropBitAtPoint(bit, ImVec2(0,0));
        src.setBit(nullptr);
        bitMovedFromTo(*bit, src, dst);

    }
}

int Chess::negamax(std::string& state, int depth, int alpha, int beta, int playerColor) 
{
    
    _countMoves++;
    if(depth == 0) {
        return evaluateBoard(state);
    }

    auto newMoves = generateAllMoves(state, playerColor);
    
    _countMoves = 0;
    int bestVal = negInfite; // Min value
    for(auto move : newMoves) {
        char  boardSave = state[move.to];
        char pieceMoving = state[move.from];

        state[move.to] = pieceMoving;
        state[move.from] = '0';
        bestVal = std::max(bestVal, -negamax(state, depth - 1, -beta, -alpha, -playerColor));

        state[move.from] = pieceMoving;
        state[move.to] = boardSave;
        alpha = std::max(alpha, bestVal);
        if(alpha >= beta) {
            break; // Beta cutoff
        }
    };

    return bestVal;
}

int Chess::evaluateBoard(const std::string& state) {
    int values[128];
    values['P'] = 10; values['p'] = -10;
    values['N'] = 30; values['n'] = -30;
    values['B'] = 30; values['b'] = -30;
    values['R'] = 50; values['r'] = -50;
    values['Q'] = 90; values['q'] = -90;
    values['K'] = 900; values['k'] = -900;

    values['0'] = 0;
    int score = 0;
    for (char ch: state) {
        score += values[ch];
    }

    return score;
}