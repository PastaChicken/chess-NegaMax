#include "Chess.h"
#include <limits>
#include <cmath>

Chess::Chess()
{
    _grid = new Grid(8, 8);
    // Precompute knight move bitboards for each square
    
    _moves = generateAllMoves();
    
}

Chess::~Chess()
{
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
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor != currentPlayer) return false;
    return true;


}
//implement movement for each piece type starting with pawns, knights, and king
//and possibly show legal moves when a piece is selected
bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
   

    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor != currentPlayer) return false;

    ChessSquare* srcSquare = static_cast<ChessSquare*>(&src);
    ChessSquare* dstSquare = static_cast<ChessSquare*>(&dst);

    int srcIndex = srcSquare->getSquareIndex();
    int dstIndex = dstSquare->getSquareIndex();

    for (auto move : _moves) {
        if (move.from == srcIndex && move.to == dstIndex) {
            return true;
        }
    }
    return false;
}



    
   /* // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor != currentPlayer) return false;

    ChessSquare* srcSquare = static_cast<ChessSquare*>(&src);
    ChessSquare* dstSquare = static_cast<ChessSquare*>(&dst);

    int srcX = srcSquare->getColumn();
    int srcY = srcSquare->getRow();
    int dstX = dstSquare->getColumn();
    int dstY = dstSquare->getRow();

    int deltaX = dstX - srcX;
    int deltaY = dstY - srcY;

    ChessPiece pieceType = static_cast<ChessPiece>(bit.gameTag() & 127);

    switch (pieceType) {
        case Pawn:
            // Pawns move forward 1 square, or 2 squares from starting position
            if (pieceColor == 0) { // White pawn
                if (deltaX == 0 && ((deltaY == 1) || (srcY == 1 && deltaY == 2 && !_grid->getSquare(dstX, dstY - 1)->bit()))) {
                    return !dstSquare->bit();
                } else if (abs(deltaX) == 1 && deltaY == 1) {
                    return dstSquare->bit() && dstSquare->bit()->getOwner() != bit.getOwner();
                }
            } else { // Black pawn
                if (deltaX == 0 && ((deltaY == -1) || (srcY == 6 && deltaY == -2 && !_grid->getSquare(dstX, dstY + 1)->bit()))) {
                    return !dstSquare->bit();
                } else if (abs(deltaX) == 1 && deltaY == -1) {
                    return dstSquare->bit() && dstSquare->bit()->getOwner() != bit.getOwner();
                }
            }
            break;
        
        case Knight:
            // Knights move in an L shape: 2 in one direction and 1 perpendicular
            //unlike pawns which can move only one direction for each player knights only need 1 if conidtion
            if ((abs(deltaX) == 2 && abs(deltaY) == 1) || (abs(deltaX) == 1 && abs(deltaY) == 2)) {
                return !dstSquare->bit() || dstSquare->bit()->getOwner() != bit.getOwner();
            }
            break;
        case King:
            // Kings move one square in any direction
            if (abs(deltaX) <= 1 && abs(deltaY) <= 1) {
                return !dstSquare->bit() || dstSquare->bit()->getOwner() != bit.getOwner();
            }
            break;
            
        default:
            return false;
    }

    return false;
    */



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
#define WHITE 1
#define BLACK -1


void Chess::generatePawnMoveList(std::vector <BitMove>& moves, const BitBoard pawns, const BitBoard emptySquares, const BitBoard enemyPieces, char color)
{
    if (pawns.getData() == 0) return;

    constexpr uint64_t NotAFile(0xfefefefefefefefeull);
    constexpr uint64_t NotHFile(0x7f7f7f7f7f7f7f7full);
    constexpr uint64_t Rank3(0x0000000000ff0000ull);
    constexpr uint64_t Rank6(0x0000ff0000000000ull);

    BitBoard demoRight(NotAFile);
    BitBoard demoLeft(NotHFile);

    BitBoard singleMoves = (color == WHITE) ? (pawns.getData() << 8) & emptySquares.getData() : (pawns.getData() >> 8) & emptySquares.getData();

    BitBoard doubleMoves = (color == WHITE) ? ((singleMoves.getData() & Rank3) << 8) & emptySquares.getData() :
                                           ((singleMoves.getData() & Rank6) >> 8) & emptySquares.getData();
                                               
    BitBoard capturesLeft = (color == WHITE) ? ((pawns.getData() & NotAFile) << 7) & enemyPieces.getData() :
                                              ((pawns.getData() & NotHFile) >> 9) & enemyPieces.getData();

    BitBoard capturesRight = (color == WHITE) ? ((pawns.getData() & NotHFile) << 9) & enemyPieces.getData() :
                                               ((pawns.getData() & NotAFile) >> 7) & enemyPieces.getData();

    int shiftFoward = (color == WHITE) ? 8 : -8;
    int doubleShift = (color == WHITE) ? 16 : -16;
    int captureLeftShift = (color == WHITE) ? 7 : -9;
    int captureRightShift = (color == WHITE) ? 9 : -7;

    addPawnBitboardMovesToList(moves, singleMoves, shiftFoward);
    addPawnBitboardMovesToList(moves, doubleMoves, doubleShift);
    addPawnBitboardMovesToList(moves, capturesLeft, captureLeftShift);
    addPawnBitboardMovesToList(moves, capturesRight, captureRightShift);
}



BitBoard Chess::generateKnightMoveBitboard(int square) {
    BitBoard bitboard;
    int rank = square / 8;
    int file = square % 8;

    std::pair<int, int> knightOffsets[] = {
        {2, 1}, {1, 2}, {-1, 2}, {-2, 1},
        {-2, -1}, {-1, -2}, {1, -2}, {2, -1}
    };

    constexpr uint64_t oneBit = 1;

    for (auto [dr, df] : knightOffsets) {
        int r = rank + dr, f = file + df;
        if (r >= 0 && r < 8 && f >= 0 && f < 8) {
            bitboard |= (oneBit << (r * 8 + f));
        }
    }
    return bitboard;
}


// Generate actual move objects from a bitboard
void Chess::generateKnightMoves(std::vector<BitMove>& moves, BitBoard knightBoard, uint64_t emptySquares) {
    knightBoard.forEachBit([&](int fromSquare) {
        BitBoard moveBitboard = BitBoard(_knightBitboards[fromSquare].getData() & emptySquares);
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Knight);
        });
    });
}

void Chess::generatePawnMoves(const char *state, std::vector<BitMove> &moves, int rown, int col, int colorAsInt) {
    const int direction = (colorAsInt == WHITE) ? 1 : -1;
    int startRow = (colorAsInt == WHITE) ? 1 : 6;


}

std::vector<BitMove> Chess::generateAllMoves()
{
    std::vector<BitMove> moves;
    moves.reserve(32);
    std::string state = stateString();
    uint64_t whiteKnights = 0LL;
    uint64_t whitePawns = 0LL;

    for( int i = 0; i < 64; i++ ) {
        if( state[i] == 'N' ) {
            whiteKnights |= (1ULL << i);
        } else if( state[i] == 'P' ) {
            whitePawns |= (1ULL << i);
        }
    }
    uint64_t occupancy = whiteKnights | whitePawns;
    generateKnightMoves(moves, BitBoard(whiteKnights), ~occupancy);
    generatePawnMoveList(moves, BitBoard(whitePawns), BitBoard(~occupancy), BitBoard(0), WHITE);
    return moves;
}