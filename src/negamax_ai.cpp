#include "negamax_ai.h"
#include <algorithm>
#include <set>
#include <cmath>
#include <climits>
#include <cstring>
#include <functional>
using namespace std;

// 评分常量
const int WIN_SCORE = 10000000;
const int LIVE_FOUR_SCORE = 300000;
const int SLEEP_FOUR_SCORE = 80000;
const int LIVE_THREE_SCORE = 20000;
const int SLEEP_THREE_SCORE = 3000;
const int LIVE_TWO_SCORE = 1000;
const int SLEEP_TWO_SCORE = 100;
const int OVERLINE_PENALTY = 5000;
const double DEFENSE_WEIGHT = 1.5;
const int CENTER_BONUS_MAX = 150;
const int CANDIDATE_RADIUS = 2;

const int DX[4] = {1, 0, 1, -1};
const int DY[4] = {0, 1, 1, 1};

NegamaxAI::NegamaxAI(int maxDepth, int numThreads) : maxDepth(maxDepth), numThreads(numThreads) {}

int NegamaxAI::getDirectionScore(const Board& b, int x, int y, int dx, int dy, int piece) const {
    int count = 1;
    int rightEmpty = 0, leftEmpty = 0;
    bool rightBlocked = false, leftBlocked = false;
    int step = 1;
    while (true) {
        int nx = x + dx * step, ny = y + dy * step;
        if (nx < 0 || nx >= BOARD_SIZE || ny < 0 || ny >= BOARD_SIZE) {
            rightBlocked = true; break;
        }
        if (b.getPiece(nx, ny) == piece) { ++count; ++step; }
        else {
            if (b.getPiece(nx, ny) == EMPTY) rightEmpty = 1;
            else rightBlocked = true;
            break;
        }
    }
    step = 1;
    while (true) {
        int nx = x - dx * step, ny = y - dy * step;
        if (nx < 0 || nx >= BOARD_SIZE || ny < 0 || ny >= BOARD_SIZE) {
            leftBlocked = true; break;
        }
        if (b.getPiece(nx, ny) == piece) { ++count; ++step; }
        else {
            if (b.getPiece(nx, ny) == EMPTY) leftEmpty = 1;
            else leftBlocked = true;
            break;
        }
    }
    if (count > 5) return -OVERLINE_PENALTY;
    if (count >= 5) return WIN_SCORE;
    int openEnds = 0;
    if (!leftBlocked && leftEmpty) openEnds++;
    if (!rightBlocked && rightEmpty) openEnds++;
    if (count == 4) {
        if (openEnds == 2) return LIVE_FOUR_SCORE;
        if (openEnds == 1) return SLEEP_FOUR_SCORE;
        return 0;
    }
    if (count == 3) {
        if (openEnds == 2) return LIVE_THREE_SCORE;
        if (openEnds == 1) return SLEEP_THREE_SCORE;
        return 0;
    }
    if (count == 2) {
        if (openEnds == 2) return LIVE_TWO_SCORE;
        if (openEnds == 1) return SLEEP_TWO_SCORE;
        return 0;
    }
    return 0;
}

int NegamaxAI::evaluatePosition(const Board& b, int x, int y, int piece) const {
    if (b.getPiece(x, y) != EMPTY) return 0;
    int totalScore = 0;
    for (int dir = 0; dir < 4; ++dir) {
        int score = getDirectionScore(b, x, y, DX[dir], DY[dir], piece);
        if (score >= WIN_SCORE) return WIN_SCORE;
        totalScore += score;
    }
    int center = BOARD_SIZE / 2;
    int dist = abs(x - center) + abs(y - center);
    int centerBonus = (BOARD_SIZE - dist) * CENTER_BONUS_MAX / BOARD_SIZE;
    totalScore += centerBonus;
    return totalScore;
}

vector<pair<int,int>> NegamaxAI::getCandidatePositions(const Board& b) const {
    set<pair<int,int>> candidates;
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            if (b.getPiece(i, j) != EMPTY) {
                for (int dx = -CANDIDATE_RADIUS; dx <= CANDIDATE_RADIUS; ++dx) {
                    for (int dy = -CANDIDATE_RADIUS; dy <= CANDIDATE_RADIUS; ++dy) {
                        int nx = i + dx, ny = j + dy;
                        if (nx>=0 && nx<BOARD_SIZE && ny>=0 && ny<BOARD_SIZE && b.getPiece(nx, ny) == EMPTY)
                            candidates.insert({nx, ny});
                    }
                }
            }
        }
    }
    if (candidates.empty()) {
        int c = BOARD_SIZE/2;
        candidates.insert({c, c});
        for (int dx=-1; dx<=1; ++dx) for (int dy=-1; dy<=1; ++dy)
            if (c+dx>=0 && c+dx<BOARD_SIZE && c+dy>=0 && c+dy<BOARD_SIZE)
                candidates.insert({c+dx, c+dy});
    }
    return vector<pair<int,int>>(candidates.begin(), candidates.end());
}

int NegamaxAI::negamax(const Board& b, int depth, int alpha, int beta, int currentPlayer, int aiPiece, int opponentPiece) const {
    int piece = (currentPlayer == 0) ? aiPiece : opponentPiece;
    int nextPlayer = 1 - currentPlayer;

    vector<pair<int,int>> candidates = getCandidatePositions(b);
    for (auto& pos : candidates) {
        if (evaluatePosition(b, pos.first, pos.second, piece) >= WIN_SCORE)
            return WIN_SCORE - depth;
    }

    if (depth == 0) {
        int best = -1e9;
        for (auto& pos : candidates) {
            int x = pos.first, y = pos.second;
            int score = evaluatePosition(b, x, y, piece);
            int oppScore = evaluatePosition(b, x, y, (piece == aiPiece) ? opponentPiece : aiPiece);
            int total = score + static_cast<int>(DEFENSE_WEIGHT * oppScore);
            if (total > best) best = total;
        }
        return best;
    }

    vector<pair<pair<int,int>, int>> scoredCandidates;
    for (auto& pos : candidates) {
        int x = pos.first, y = pos.second;
        int score = evaluatePosition(b, x, y, piece);
        int oppScore = evaluatePosition(b, x, y, (piece == aiPiece) ? opponentPiece : aiPiece);
        scoredCandidates.push_back({pos, score + static_cast<int>(DEFENSE_WEIGHT * oppScore)});
    }
    sort(scoredCandidates.begin(), scoredCandidates.end(),
         [](const pair<pair<int,int>, int>& a, const pair<pair<int,int>, int>& b) { return a.second > b.second; });

    int value = -1e9;
    for (auto& sc : scoredCandidates) {
        int x = sc.first.first, y = sc.first.second;
        Board newBoard = b;
        newBoard.setPiece(x, y, piece);
        int childValue = -negamax(newBoard, depth-1, -beta, -alpha, nextPlayer, aiPiece, opponentPiece);
        value = max(value, childValue);
        alpha = max(alpha, value);
        if (alpha >= beta) break;
    }
    return value;
}

// ThreadPool implementation
NegamaxAI::ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back([this] {
            while (true) {
                function<void()> task;
                {
                    unique_lock<mutex> lock(queueMutex);
                    condition.wait(lock, [this] { return stop || !tasks.empty(); });
                    if (stop && tasks.empty()) return;
                    task = move(tasks.front());
                    tasks.pop();
                }
                task();
            }
        });
    }
}

NegamaxAI::ThreadPool::~ThreadPool() {
    {
        unique_lock<mutex> lock(queueMutex);
        stop = true;
    }
    condition.notify_all();
    for (thread& worker : workers) worker.join();
}

template<class F, class... Args>
auto NegamaxAI::ThreadPool::enqueue(F&& f, Args&&... args) -> future<typename result_of<F(Args...)>::type> {
    using return_type = typename result_of<F(Args...)>::type;
    auto task = make_shared<packaged_task<return_type()>>(bind(forward<F>(f), forward<Args>(args)...));
    future<return_type> res = task->get_future();
    {
        unique_lock<mutex> lock(queueMutex);
        if (stop) throw runtime_error("enqueue on stopped ThreadPool");
        tasks.emplace([task]() { (*task)(); });
    }
    condition.notify_one();
    return res;
}

pair<int,int> NegamaxAI::getMove(const Board& board, int aiPiece, int opponentPiece) {
    vector<pair<int,int>> candidates = getCandidatePositions(board);
    if (candidates.empty()) return {-1, -1};

    // 快速胜利/防守
    for (auto& pos : candidates) {
        if (evaluatePosition(board, pos.first, pos.second, aiPiece) >= WIN_SCORE) return pos;
        if (evaluatePosition(board, pos.first, pos.second, opponentPiece) >= WIN_SCORE) return pos;
    }

    ThreadPool pool(numThreads);
    vector<future<CandidateResult>> futures;
    for (auto& pos : candidates) {
        Board boardCopy = board;
        int x = pos.first, y = pos.second;
        int depth = maxDepth;
        auto fut = pool.enqueue([this, boardCopy, x, y, depth, aiPiece, opponentPiece]() -> CandidateResult {
            Board localBoard = boardCopy;
            localBoard.setPiece(x, y, aiPiece);
            int value = -negamax(localBoard, depth-1, -1e9, 1e9, 1, aiPiece, opponentPiece);
            int oppScore = evaluatePosition(boardCopy, x, y, opponentPiece);
            int total = value + static_cast<int>(DEFENSE_WEIGHT * oppScore);
            return { {x, y}, total };
        });
        futures.push_back(move(fut));
    }
    int bestValue = -1e9;
    pair<int,int> bestPos = candidates[0];
    for (auto& fut : futures) {
        CandidateResult res = fut.get();
        if (res.value > bestValue) {
            bestValue = res.value;
            bestPos = res.pos;
        }
    }
    return bestPos;
}