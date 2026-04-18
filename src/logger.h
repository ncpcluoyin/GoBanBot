#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>

class Logger {
public:
    static Logger& instance();
    void log(const std::string& msg);
    void logMove(const std::string& model, int x, int y, double eval, int sims, int cacheHits, double timeMs);

private:
    Logger();
    ~Logger();
    std::ofstream file;
    std::mutex mtx;
};

#endif