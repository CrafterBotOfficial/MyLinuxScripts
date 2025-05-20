#include <iostream>

enum command {
    pull,
    push,
    install
};

std::string saveDirectory;

void clone(const std::string source, const std::string path);
void executePush(auto targets);
void executePull(auto targets);
void installPackages(auto packages);
std::string execute(std::string command);
std::string getCopyPath(std::string source);
