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
    _bitBoardLookup['W'] = WHITE_PAWNS;
    _bitBoardLookup['N'] = WHITE_KNIGHTS;
    _bitBoardLookup['B'] = WHITE_BISHOPS;
    _bitBoardLookup['R'] = WHITE_ROOKS;
    _bitBoardLookup['Q'] = WHITE_QUEENS;
    _bitBoardLookup['K'] = WHITE_KING;
    _bitBoardLookup['w'] = BLACK_PAWNS;
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
    if (sourceSquare) {
        int squareIndex = sourceSquare->getSquareIndex();
        for(auto move :_moves) {
            if(move.from == squareIndex) {
                ret = true;
                auto dest = _grid->getSquareByIndex(move.to);
                dest->setHighlighted(true);
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
   if(pawns.getData() == 0) return;
    
    
    
    BitBoard demoLeft(NotAFile);
    BitBoard demoRight(NotHFile);

    //calculate single pawn moves foward
    BitBoard singleMoves = (color == WHITE) ? (pawns.getData() << 8) & emptySquares.getData() : (pawns.getData() >> 8) & emptySquares.getData();

    BitBoard doubleMoves = (color == WHITE) ? ((singleMoves.getData() & Rank3) << 8) & emptySquares.getData() : ((singleMoves.getData() & Rank6) >> 8) & emptySquares.getData();

    //calculate left pawn captures
    BitBoard capturesLeft = (color == WHITE) ? ((pawns.getData() & NotAFile) << 7) & enemyPieces.getData() : ((pawns.getData() & NotAFile) >> 9) & enemyPieces.getData();

    //calculate right pawn captures
    BitBoard capturesRight = (color == WHITE) ? ((pawns.getData() & NotHFile) << 9) & enemyPieces.getData() : ((pawns.getData() & NotHFile) >> 7) & enemyPieces.getData();

    int shiftForward = (color == WHITE) ? 8 : -8;
    int doubleShift = (color == WHITE) ? 16 : -16;
    int captureLeftShift = (color == WHITE) ? 7 : -9;
    int captureRightShift = (color == WHITE) ? 9 : -7;

    
}

/*
BitBoard singleMoves = (color == BLACK) ? (pawns.getData() << 8) & emptySquares.getData() : (pawns.getData() >> 8) & emptySquares.getData();
    // Calculate double pawn moves from starting rank
    BitBoard doubleMoves = (color == BLACK) ? ((singleMoves.getData() & Rank3) << 8) & emptySquares.getData() : ((singleMoves.getData() & Rank6) >> 8) & emptySquares.getData();
    // Calculate left and right pawn captures
    BitBoard capturesLeft = (color == BLACK) ? ((pawns.getData() & NotAFile) << 7) & enemyPieces.getData() : ((pawns.getData() & NotAFile) >> 9) & enemyPieces.getData();
    BitBoard capturesRight = (color == BLACK) ? ((pawns.getData() & NotHFile) << 9) & enemyPieces.getData() : ((pawns.getData() & NotHFile) >> 7) & enemyPieces.getData();
*/


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
    int oppBitIndex = playerColor == WHITE ? BLACK_PAWNS : WHITE_PAWNS;

    generateKnightMoves(moves, _bitboards[WHITE_KNIGHTS + bitIndex], ~_bitboards[OCCUPANCY].getData());
    generateKingMoves(moves, _bitboards[WHITE_KING + bitIndex], ~_bitboards[WHITE_ALL_PIECEES + oppBitIndex].getData());
    generateBishopMoves(moves, _bitboards[WHITE_BISHOPS + bitIndex], _bitboards[OCCUPANCY].getData(), _bitboards[WHITE_ALL_PIECEES + bitIndex].getData());
    generatePawnMoveList(moves, _bitboards[WHITE_PAWNS + bitIndex], _bitboards[OCCUPANCY].getData(), _bitboards[WHITE_ALL_PIECEES + oppBitIndex].getData(), playerColor);
    generateRookMoves(moves, _bitboards[WHITE_ROOKS + bitIndex], _bitboards[OCCUPANCY].getData(), _bitboards[WHITE_ALL_PIECEES + bitIndex].getData());
    generateQueenMoves(moves, _bitboards[WHITE_QUEENS + bitIndex], _bitboards[OCCUPANCY].getData(), _bitboards[WHITE_ALL_PIECEES + bitIndex].getData());
    return moves;
}

void Chess::updateAI() {
    
}