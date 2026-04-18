#include "mcts_ai.h"
#include "logger.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <future>
#include <chrono>

using namespace std;
using namespace Eigen;   // 关键：添加 Eigen 命名空间

// ---------- MCTSNode ----------
MCTSNode::MCTSNode(const Board& b, int x, int y, int player, MCTSNode* p)
    : state(b), moveX(x), moveY(y), playerJustMoved(player), parent(p),
      visits(0), totalValue(0.0f), isExpanded(false) {}

MCTSNode::~MCTSNode() {
    for (auto* child : children) delete child;
}

vector<pair<int,int>> getLegalMoves(const Board& b) {
    vector<pair<int,int>> moves;
    for (int i=0; i<BOARD_SIZE; ++i)
        for (int j=0; j<BOARD_SIZE; ++j)
            if (b.getPiece(i,j)==EMPTY) moves.push_back({i,j});
    return moves;
}

void MCTSNode::expand(const Board& board, int aiPiece, int opponentPiece) {
    lock_guard<mutex> lock(nodeMutex);
    if (isExpanded) return;
    auto moves = getLegalMoves(board);
    for (auto& mv : moves) {
        Board newState = board;
        int nextPlayer = (playerJustMoved == aiPiece) ? opponentPiece : aiPiece;
        newState.setPiece(mv.first, mv.second, nextPlayer);
        children.push_back(new MCTSNode(newState, mv.first, mv.second, nextPlayer, this));
    }
    isExpanded = true;
}

float MCTSNode::uctValue(int totalVisits, float exploration) const {
    if (visits == 0) return 1e9f;
    float winRate = totalValue / visits;
    return winRate + exploration * sqrt(log(totalVisits) / visits);
}

MCTSNode* MCTSNode::bestChild(float exploration) const {
    MCTSNode* best = nullptr;
    float bestVal = -1e9f;
    for (auto* child : children) {
        float val = child->uctValue(visits, exploration);
        if (val > bestVal) { bestVal = val; best = child; }
    }
    return best;
}

void MCTSNode::update(float value) {
    lock_guard<mutex> lock(nodeMutex);
    visits++;
    totalValue += value;
}

// ---------- MCTSAI ----------
MCTSAI::MCTSAI(int sims, int threads, const string& weightsFile, int in, int h1, int h2)
    : numSimulations(sims), numThreads(threads) {
    net = make_unique<NeuralNet>(in, h1, h2);
    if (!weightsFile.empty()) net->loadWeights(weightsFile);
}

MCTSAI::~MCTSAI() {}

vector<pair<int,int>> MCTSAI::getLegalMoves(const Board& board) const {
    vector<pair<int,int>> moves;
    for (int i=0; i<BOARD_SIZE; ++i)
        for (int j=0; j<BOARD_SIZE; ++j)
            if (board.getPiece(i,j) == EMPTY) moves.push_back({i,j});
    return moves;
}

float MCTSAI::evaluateState(const Board& board, int currentPlayer, int aiPiece, int opponentPiece) const {
    VectorXf input(net->getInputSize());
    int idx=0;
    for (int i=0; i<BOARD_SIZE; ++i)
        for (int j=0; j<BOARD_SIZE; ++j) {
            int p = board.getPiece(i,j);
            if (p == EMPTY) input(idx) = 0;
            else if (p == currentPlayer) input(idx) = 1;
            else input(idx) = -1;
            idx++;
        }
    return net->forward(input);
}

MCTSNode* MCTSAI::runSimulation(MCTSNode* root, int aiPiece, int opponentPiece) {
    MCTSNode* node = root;
    // Selection
    while (node->isExpanded && !node->children.empty()) {
        node = node->bestChild(1.414f);
    }
    // Expansion
    if (!node->isExpanded && node->visits > 0) {
        node->expand(node->state, aiPiece, opponentPiece);
        if (!node->children.empty()) {
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<> dis(0, node->children.size()-1);
            node = node->children[dis(gen)];
        }
    }
    // Evaluation
    int nextPlayer = (node->playerJustMoved == aiPiece) ? opponentPiece : aiPiece;
    float value = evaluateState(node->state, nextPlayer, aiPiece, opponentPiece);
    return node;
}

void MCTSAI::backpropagate(MCTSNode* node, float value) {
    while (node != nullptr) {
        node->update(value);
        value = 1.0f - value;
        node = node->parent;
    }
}

AIResult MCTSAI::getMove(const Board& board, int aiPiece, int opponentPiece, int timeLimitMs) {
    auto start = chrono::steady_clock::now();
    MCTSNode* root = new MCTSNode(board, -1, -1, opponentPiece, nullptr);
    vector<future<MCTSNode*>> futures;
    for (int i=0; i<numSimulations; ++i) {
        futures.emplace_back(async(launch::async, [this, root, aiPiece, opponentPiece]() {
            auto* leaf = runSimulation(root, aiPiece, opponentPiece);
            float value = evaluateState(leaf->state, leaf->playerJustMoved, aiPiece, opponentPiece);
            backpropagate(leaf, value);
            return leaf;
        }));
    }
    for (auto& fut : futures) fut.wait();

    MCTSNode* best = nullptr;
    int bestVisits = -1;
    for (auto* child : root->children) {
        if (child->visits > bestVisits) {
            bestVisits = child->visits;
            best = child;
        }
    }
    auto end = chrono::steady_clock::now();
    double timeMs = chrono::duration<double, milli>(end-start).count();

    AIResult res;
    if (best) {
        res.x = best->moveX; res.y = best->moveY;
        res.evaluation = best->totalValue / best->visits;
    } else {
        res.x = -1; res.y = -1;
        res.evaluation = 0;
    }
    res.simulations = numSimulations;
    res.cacheHits = 0;
    res.timeMs = timeMs;
    delete root;
    return res;
}

void MCTSAI::saveModel(const string& filename) const {
    net->saveWeights(filename);
    string paramFile = filename + ".params";
    net->saveParams(paramFile);
}

void MCTSAI::loadModel(const string& filename) {
    string paramFile = filename + ".params";
    net->loadParams(paramFile);
    net->loadWeights(filename);
}