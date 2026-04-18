#include "negamax_ai.h"
#include "logger.h"
#include <algorithm>
#include <set>
#include <cmath>
#include <climits>
#include <thread>
#include <future>

using namespace std;

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

const int DX[4] = {1,0,1,-1};
const int DY[4] = {0,1,1,1};

NegamaxAI::NegamaxAI(int depth, int threads) : maxDepth(depth), numThreads(threads) {}

int NegamaxAI::getDirectionScore(const Board& b, int x, int y, int dx, int dy, int piece) const {
    int cnt = 1;
    int rightEmpty=0, leftEmpty=0;
    bool rightBlocked=false, leftBlocked=false;
    int step=1;
    while (true) {
        int nx = x + dx*step, ny = y + dy*step;
        if (nx<0||nx>=BOARD_SIZE||ny<0||ny>=BOARD_SIZE) { rightBlocked=true; break; }
        if (b.getPiece(nx,ny)==piece) { cnt++; step++; }
        else {
            if (b.getPiece(nx,ny)==EMPTY) rightEmpty=1;
            else rightBlocked=true;
            break;
        }
    }
    step=1;
    while (true) {
        int nx = x - dx*step, ny = y - dy*step;
        if (nx<0||nx>=BOARD_SIZE||ny<0||ny>=BOARD_SIZE) { leftBlocked=true; break; }
        if (b.getPiece(nx,ny)==piece) { cnt++; step++; }
        else {
            if (b.getPiece(nx,ny)==EMPTY) leftEmpty=1;
            else leftBlocked=true;
            break;
        }
    }
    if (cnt>5) return -OVERLINE_PENALTY;
    if (cnt>=5) return WIN_SCORE;
    int openEnds=0;
    if (!leftBlocked && leftEmpty) openEnds++;
    if (!rightBlocked && rightEmpty) openEnds++;
    if (cnt==4) {
        if (openEnds==2) return LIVE_FOUR_SCORE;
        if (openEnds==1) return SLEEP_FOUR_SCORE;
        return 0;
    }
    if (cnt==3) {
        if (openEnds==2) return LIVE_THREE_SCORE;
        if (openEnds==1) return SLEEP_THREE_SCORE;
        return 0;
    }
    if (cnt==2) {
        if (openEnds==2) return LIVE_TWO_SCORE;
        if (openEnds==1) return SLEEP_TWO_SCORE;
        return 0;
    }
    return 0;
}

int NegamaxAI::evaluatePosition(const Board& b, int x, int y, int piece) const {
    if (b.getPiece(x,y)!=EMPTY) return 0;
    int total=0;
    for (int dir=0; dir<4; ++dir) {
        int sc = getDirectionScore(b, x, y, DX[dir], DY[dir], piece);
        if (sc>=WIN_SCORE) return WIN_SCORE;
        total += sc;
    }
    int center = BOARD_SIZE/2;
    int dist = abs(x-center)+abs(y-center);
    total += (BOARD_SIZE-dist)*CENTER_BONUS_MAX/BOARD_SIZE;
    return total;
}

vector<pair<int,int>> NegamaxAI::getCandidatePositions(const Board& b) const {
    set<pair<int,int>> cand;
    for (int i=0; i<BOARD_SIZE; ++i)
        for (int j=0; j<BOARD_SIZE; ++j)
            if (b.getPiece(i,j)!=EMPTY)
                for (int dx=-CANDIDATE_RADIUS; dx<=CANDIDATE_RADIUS; ++dx)
                    for (int dy=-CANDIDATE_RADIUS; dy<=CANDIDATE_RADIUS; ++dy) {
                        int ni=i+dx, nj=j+dy;
                        if (ni>=0 && ni<BOARD_SIZE && nj>=0 && nj<BOARD_SIZE && b.getPiece(ni,nj)==EMPTY)
                            cand.insert({ni,nj});
                    }
    if (cand.empty()) {
        int c=BOARD_SIZE/2;
        cand.insert({c,c});
    }
    return vector<pair<int,int>>(cand.begin(), cand.end());
}

void NegamaxAI::sortMoves(const Board& b, vector<pair<int,int>>& moves, int piece) const {
    // 简化，实际需要按评分排序，但negamax中会动态排序
}

int NegamaxAI::negamax(const Board& b, int depth, int alpha, int beta, int currentPlayer,
                       int aiPiece, int opponentPiece, ZobristKey hash, int& nodes) {
    nodes++;
    int piece = (currentPlayer==0) ? aiPiece : opponentPiece;
    int nextPlayer = 1-currentPlayer;

    auto it = transTable.find(hash);
    if (it != transTable.end() && it->second.depth >= depth) {
        cacheHits++;
        TTEntry& entry = it->second;
        if (entry.flag == 0) return entry.value;
        else if (entry.flag == 1 && entry.value >= beta) return entry.value;
        else if (entry.flag == 2 && entry.value <= alpha) return entry.value;
    }

    auto cand = getCandidatePositions(b);
    for (auto& mv : cand) {
        if (evaluatePosition(b, mv.first, mv.second, piece) >= WIN_SCORE) {
            TTEntry entry{depth, WIN_SCORE-depth, 0, hash};
            transTable[hash] = entry;
            return WIN_SCORE - depth;
        }
    }

    if (depth==0) {
        int best = -1e9;
        for (auto& mv : cand) {
            int sc = evaluatePosition(b, mv.first, mv.second, piece);
            int oppSc = evaluatePosition(b, mv.first, mv.second, (piece==aiPiece)?opponentPiece:aiPiece);
            int total = sc + DEFENSE_WEIGHT * oppSc;
            if (total>best) best=total;
        }
        return best;
    }

    vector<pair<pair<int,int>, int>> scoredCand;
    for (auto& mv : cand) {
        int sc = evaluatePosition(b, mv.first, mv.second, piece);
        int oppSc = evaluatePosition(b, mv.first, mv.second, (piece==aiPiece)?opponentPiece:aiPiece);
        scoredCand.push_back({mv, sc + static_cast<int>(DEFENSE_WEIGHT*oppSc)});
    }
    sort(scoredCand.begin(), scoredCand.end(),
         [](const auto& a, const auto& b) { return a.second > b.second; });

    int value = -1e9;
    int flag = 2;
    for (auto& sc : scoredCand) {
        int x=sc.first.first, y=sc.first.second;
        Board newb = b;
        newb.setPiece(x, y, piece);
        ZobristKey newHash = newb.getZobristHash();
        int childValue = -negamax(newb, depth-1, -beta, -alpha, nextPlayer,
                                   aiPiece, opponentPiece, newHash, nodes);
        if (childValue > value) value = childValue;
        alpha = max(alpha, value);
        if (alpha >= beta) {
            flag = 1;
            break;
        }
    }
    TTEntry entry{depth, value, (value<=alpha?2:(value>=beta?1:0)), hash};
    transTable[hash] = entry;
    return value;
}

AIResult NegamaxAI::getMove(const Board& board, int aiPiece, int opponentPiece, int timeLimitMs) {
    auto cand = getCandidatePositions(board);
    if (cand.empty()) return {-1,-1,0.0,0,0,0.0};

    cacheHits = 0;
    int nodes = 0;
    auto start = chrono::steady_clock::now();
    int bestVal = -1e9;
    int bestX=-1, bestY=-1;
    for (auto& mv : cand) {
        Board newb = board;
        newb.setPiece(mv.first, mv.second, aiPiece);
        int val = -negamax(newb, maxDepth-1, -1e9, 1e9, 1,
                           aiPiece, opponentPiece, newb.getZobristHash(), nodes);
        if (val > bestVal) {
            bestVal = val;
            bestX = mv.first; bestY = mv.second;
        }
    }
    auto end = chrono::steady_clock::now();
    double timeMs = chrono::duration<double, milli>(end-start).count();

    AIResult res;
    res.x = bestX; res.y = bestY;
    res.evaluation = bestVal / (double)WIN_SCORE;
    res.simulations = nodes;
    res.cacheHits = cacheHits;
    res.timeMs = timeMs;
    return res;
}