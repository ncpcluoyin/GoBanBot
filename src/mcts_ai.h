#ifndef MCTS_AI_H
#define MCTS_AI_H

#include "ai_interface.h"
#include "board.h"
#include "neural_net.h"
#include <memory>
#include <atomic>
#include <mutex>
#include <vector>
#include <thread>

struct MCTSNode {
    Board state;
    int moveX, moveY;
    int playerJustMoved;
    MCTSNode* parent;
    std::vector<MCTSNode*> children;
    int visits;
    float totalValue;
    bool isExpanded;
    std::mutex nodeMutex;

    MCTSNode(const Board& b, int x, int y, int player, MCTSNode* p=nullptr);
    ~MCTSNode();
    void expand(const Board& board, int aiPiece, int opponentPiece);
    float uctValue(int totalVisits, float exploration) const;
    MCTSNode* bestChild(float exploration) const;
    void update(float value);
};

class MCTSAI : public GomokuAI {
public:
    MCTSAI(int simulations, int threads, const std::string& weightsFile = "",
           int inputSize=225, int hidden1=128, int hidden2=64);
    ~MCTSAI();
    AIResult getMove(const Board& board, int aiPiece, int opponentPiece, int timeLimitMs=0) override;
    void saveModel(const std::string& filename) const;
    void loadModel(const std::string& filename);

private:
    int numSimulations;
    int numThreads;
    std::unique_ptr<NeuralNet> net;
    std::vector<std::pair<int,int>> getLegalMoves(const Board& board) const;
    float evaluateState(const Board& board, int currentPlayer, int aiPiece, int opponentPiece) const;
    MCTSNode* runSimulation(MCTSNode* root, int aiPiece, int opponentPiece);
    void backpropagate(MCTSNode* node, float value);
};

#endif