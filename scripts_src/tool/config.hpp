#include "lib/json/single_include/nlohmann/json.hpp"

enum operation {
    pull,
    push,
    compile,
    install
};
class Config {
    public:
        bool verbose;
        operation mode;
        nlohmann::json json;
        std::filesystem::path workingDirectory;
};