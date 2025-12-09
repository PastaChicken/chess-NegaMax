#pragma once

#include "Game.h"
#include "Grid.h"
#include "BitBoard.h"

constexpr int pieceSize = 80;
constexpr int WHITE = +1;
constexpr int BLACK = -1;
constexpr uint64_t NotAFile(0xFEFEFEFEFEFEFEFEULL); //A file mask
constexpr uint64_t NotHFile(0x7F7F7F7F7F7F7F7FULL); //H file mask
constexpr uint64_t Rank3(0x0000000000FF0000ULL); //Rank 3 mask
constexpr uint64_t Rank6(0x0000FF0000000000ULL); //Rank 6 mask
constexpr uint64_t Rank2(0x000000000000FF00); //Rank 2 mask (white pawns start)
constexpr uint64_t Rank7(0x00FF000000000000); //Rank 7 mask (black pawns start)
constexpr int negInfite = -100000;
constexpr int posInfite = +100000;

enum AllBitBoards {
    WHITE_PAWNS,
    WHITE_KNIGHTS,
    WHITE_BISHOPS,
    WHITE_ROOKS,
    WHITE_QUEENS,
    WHITE_KING,
    BLACK_PAWNS,
    BLACK_KNIGHTS,
    BLACK_BISHOPS,
    BLACK_ROOKS,
    BLACK_QUEENS,
    BLACK_KING,
    WHITE_ALL_PIECEES,
    BLACK_ALL_PIECES,
    OCCUPANCY,
    EMPTY_SQUARES,
    eNUM_BITBOARDS
};



class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    void bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;
    bool        gameHasAI() override { return true; }

    Grid* getGrid() override { return _grid; }
    void updateAI() override;

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;
    BitBoard generateKnightMoveBitboard(int square);
    void generateKnightMoves(std::vector<BitMove>& moves, BitBoard knightBoard, uint64_t emptySquares);
    void generateKingMoves(std::vector<BitMove>& moves, BitBoard kingBoard, uint64_t occupancy);
    void generatePawnMoves(std::vector<BitMove>& moves, BitBoard pawnBoard, uint64_t occupancy, uint64_t enemyPieces, char color);
    void addPawnBitboardMovesToList(std::vector<BitMove>& moves,  BitBoard bitboard, const int direction);
    void generatePawnMoveList(std::vector<BitMove>& moves, const BitBoard pawns, const BitBoard emptySquares, const BitBoard enemyPieces, char color);
    void generateBishopMoves(std::vector<BitMove>& moves, BitBoard bishopBoard, uint64_t occupancy, uint64_t friendlies);
    void generateRookMoves(std::vector<BitMove>& moves, BitBoard rookBoard, uint64_t occupancy, uint64_t friendlies);
    void generateQueenMoves(std::vector<BitMove>& moves, BitBoard queenBoard, uint64_t occupancy, uint64_t friendlies);

    std::vector<BitMove> generateAllMoves(const std::string& stateString, int playerColor);

    int negamax(std::string& state, int depth, int alpha, int beta, int playerColor);
    
    int evaluateBoard(const std::string& board);

    inline int  bitScanForward(uint64_t bb) const {
    #if defined(_MSC_VER) && !defined(__clang__)
        unsigned long index;
        _BitScanForward64(&index, bb);
        return index;
    #else
        return __builtin_ffsll(bb) - 1;
    #endif
    }
    int _currentPlayer;
    int _countMoves;

    Grid* _grid;
    BitBoard _bitboards[eNUM_BITBOARDS];
    std::vector<BitMove> _moves;
    int _bitBoardLookup[128];
};