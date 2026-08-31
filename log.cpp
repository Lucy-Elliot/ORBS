#include "log.h"
#include <vector>
#include <string>


std::vector<std::string> consoleLog;

void Log(const std::string& msg) {
    consoleLog.push_back(msg);
    if (consoleLog.size() > 8) consoleLog.erase(consoleLog.begin());
}
