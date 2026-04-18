#include "logger.h"
#include <iostream>
#include <ctime>
#include <iomanip>

using namespace std;

Logger::Logger() {
    file.open("logs/gobanbot.log", ios::app);
    if (!file) {
        cerr << "Warning: Cannot open log file" << endl;
    }
}

Logger::~Logger() {
    if (file.is_open()) file.close();
}

Logger& Logger::instance() {
    static Logger log;
    return log;
}

void Logger::log(const string& msg) {
    lock_guard<mutex> lock(mtx);
    if (!file.is_open()) return;
    auto now = chrono::system_clock::now();
    auto t = chrono::system_clock::to_time_t(now);
    file << put_time(localtime(&t), "%Y-%m-%d %H:%M:%S") << " " << msg << endl;
}

void Logger::logMove(const string& model, int x, int y, double eval, int sims, int cacheHits, double timeMs) {
    string msg = model + " move at (" + to_string(x+1) + "," + to_string(y+1) + ")";
    msg += " eval=" + to_string(eval) + " sims=" + to_string(sims);
    msg += " cacheHits=" + to_string(cacheHits) + " time=" + to_string(timeMs) + "ms";
    log(msg);
}