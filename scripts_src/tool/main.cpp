#include "main.hpp"
#include "lib/json.hpp"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    printf("Working...\n");

    printf("Reading config.json\n");
    
    FILE *config = fopen("config.json", "r");
    if (config == nullptr) {
        printf("Config.json not found, please make sure your running this program in the same directory as it.\n");
        return 1;
    }
    auto json = nlohmann::json::parse(config);
    auto targets = json["targets"];
    saveDirectory = json["directory"];

    printf("Done!\n");

    if (argc != 2) {
help:
        int packageCount = json["packages"].size();
        std::cout 
            << "\nCommands:\n" 
            << "    push - Replaces all config files on machine with ones the in " << saveDirectory << " (dangerous)\n"
            << "    pull - Copies all config files on machine to " << saveDirectory << "\n" 
            << "    install - Installs all packages specified in config (" << packageCount << " packages)\n"
            << "    compile - Compiles c/c++ scripts within scripts_src and installs them.\n"
            << "\n"; 
        return 1;
    }

    static const std::map<std::string, command> commands {
        { "push", push },
        { "pull", pull },
        { "install", install },
        { "compile", compile }
    };
    auto cmd = commands.find(argv[1]); 
    if (cmd == commands.end()) goto help;

    switch (cmd->second) {
        case push:
            executePush(targets);
            printf("Reloading hyprland\n");
            execute("hyprctl reload");
            break;
        case install: 
            std::cout << "Installing packages...\n";
            installPackages(json["packages"], json["hypr-plugins"]);
            break;
        case pull: 
            printf("Pulling to save directory...");
            executePull(targets);
            break;
        case compile: 
            printf("Compiling scripts_src...\n");
            compileTargets(json["compile"]);
            break;
    }

    printf("Finished\n");
    return 0;
}

void compileTargets(auto targets) {
    fs::create_directories("scripts");

    for (auto target : targets) {
        std::string name = target["target"];
        auto command = target["tool"];
        std::cout << "Compiling " << name << "\n";
        
        fs::path path(name);
        execute(std::string(command) + " " + std::string(name) + " -o " + path.filename().string());
        fs::rename(path.filename().string(), "scripts/" + path.parent_path().filename().string());

        std::cout << "Done\n";
    }
}

void executePull(auto targets) {
    std::cout << "Pulling dotfiles...\n";
    if (fs::exists(saveDirectory) && fs::is_directory(saveDirectory)) {
        fs::remove_all(saveDirectory);
    }
    else std::cout << "Couldn't find or remove dotfiles\n";
    
    if (!fs::create_directories(saveDirectory)) {
        std::cout << "Failed to create dotfiles holding directory";
        return;
    } 

    for (auto path : targets) {
        std::cout << "Pulling " << path << "\n";
        if (!std::filesystem::exists(path)) {
            std::cout << "Not found" << "\n";
            continue;
        }

        auto copyPath = getCopyPath(path);
        std::cout << copyPath << "\n";
        clone(path, copyPath);
    }
}

void executePush(auto targets) {
    for (auto path : targets) {
        auto localPath = getCopyPath(path);
        
        char input;
        std::cout << "Pushing " << localPath << " to " << path << " Continue?[y/n]" << "\n";
        std::cin >> input; 

        if (input != 'y' && input != 'Y') { 
            std::cout << "\nCanceling...\n";
            break;
        }

        clone(localPath, path);
    }
}

void installPackages(auto packages, auto plugins) {
    std::cout << "Are you sure? [y/n]: ";
    char input;
    std::cin >> input;
    if (input != 'y') {
        std::cout << "Aborting...";
        return;
    }

    std::cout << "Updating package list...\n";
    execute(std::string("sudo pacman -Sy"));
    std::cout << "Done\n";

    int packagesInstalled = 0;
    for (std::string package : packages) 
        try {
            auto query = execute("pacman -Q " + package);
            auto isUpgradeableQuery = execute("pacman -Qu " + package);
            if (!query.empty() && isUpgradeableQuery.empty()) {
                std::cout << package << " already installed!\n";
                continue;
            }

            std::string lookUpQuery = execute("pacman -Si " + package);
            if (lookUpQuery.empty()) {
                std::cout << package << " is a AUR package, installing with yay...\n";
                execute("yay -S " + package + " --noconfirm --verbose");
                goto finished;
            }

            std::cout << "Installing " << package << "\n";
            execute("sudo pacman -S " + package + " --noconfirm");
finished:
            std::cout << "Done\n";
            packagesInstalled++;
        } catch (...) {
            std::cout << "Failed to install package " << package << "\n";
        }

        std::cout << "Installing Hyprland plugins\n";
        execute("hyprpm update");
        std::cout << "Updated";

        for (std::string plugin : plugins) {
            std::cout << "Installing " << plugin << "\n";
            system(std::string("hyprpm add " + plugin).c_str());
            execute("hyprpm enable " + plugin.substr(plugin.find_last_of('/') + 1));
        }

    std::cout << (packagesInstalled == 0 ? "No packages installed" : "Installed " + std::to_string(packagesInstalled) + " packages!") << "\n";
}

void clone(std::string source, std::string path) {
    if (path.starts_with("/etc/") || path.starts_with("/bin/")) {
        std::cout << "Copying to " << path << " as root\n";
        std::string command = "sudo cp -r " + source + " " + path;
        execute(command);
        return;
    }
    
    fs::remove_all(path);
    if (fs::is_directory(source)) fs::copy(source, path, fs::copy_options::recursive);
    else fs::copy(source, path);
}

std::string execute(std::string command) {
    char buffer[128];
    std::string output = "";
    FILE* pipe = popen(command.c_str(), "r");

    while (fgets(buffer, sizeof buffer, pipe) != NULL) output += buffer;

    return output;
}

std::string getCopyPath(std::string source) {
    return saveDirectory + "/" + std::filesystem::path(source).filename().string();
}
