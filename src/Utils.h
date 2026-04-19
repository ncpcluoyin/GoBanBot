#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>

inline void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

inline char toLower(char c) { return std::tolower(static_cast<unsigned char>(c)); }

inline std::string toLower(const std::string& s) {
    std::string res = s;
    std::transform(res.begin(), res.end(), res.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return res;
}

inline bool yesNoPrompt(const std::string& question) {
    std::cout << question << " (y/n): ";
    std::string ans;
    std::cin >> ans;
    ans = toLower(ans);
    return (!ans.empty() && ans[0] == 'y');
}

inline int intPrompt(const std::string& question, int minVal, int maxVal) {
    int val;
    while (true) {
        std::cout << question;
        if (maxVal < 0) std::cout << " [" << minVal << "+]";
        else std::cout << " [" << minVal << "-" << maxVal << "]";
        std::cout << ": ";
        std::cin >> val;
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Invalid input. Please enter a number.\n";
            continue;
        }
        if (val >= minVal && (maxVal < 0 || val <= maxVal)) break;
        std::cout << "Value out of range. Try again.\n";
    }
    return val;
}

#endif