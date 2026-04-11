#ifndef NEGAMAX_AI_H
#define NEGAMAX_AI_H

#include "ai_interface.h"
#include "board.h"
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <future>

class NegamaxAI : public GomokuAI {
public:
    NegamaxAI(int maxDepth, int numThreads);
    std::pair<int, int> getMove(const Board& board, int aiPiece, int opponentPiece) override;

private:
    int maxDepth;
    int numThreads;

    struct CandidateResult {
        std::pair<int,int> pos;
        int value;
    };

    int evaluatePosition(const Board& b, int x, int y, int piece) const;
    int getDirectionScore(const Board& b, int x, int y, int dx, int dy, int piece) const;
    std::vector<std::pair<int,int>> getCandidatePositions(const Board& b) const;
    int negamax(const Board& b, int depth, int alpha, int beta, int currentPlayer, int aiPiece, int opponentPiece) const;

    class ThreadPool {
    public:
        ThreadPool(size_t threads);
        ~ThreadPool();
        template<class F, class... Args>
        auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;
    private:
        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;
        std::mutex queueMutex;
        std::condition_variable condition;
        bool stop;
    };
};

#endif // NEGAMAX_AI_H