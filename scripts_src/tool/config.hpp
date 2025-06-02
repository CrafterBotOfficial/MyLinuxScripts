#include "lib/json/single_include/nlohmann/json.hpp"

enum operation {
    pull = 1,
    push,
    compile,
    install
};
class Config {
    public:
        bool verbose;
        operation mode = (operation)0;
        nlohmann::json json;
        std::filesystem::path workingDirectory;
};