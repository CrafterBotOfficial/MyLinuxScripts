#include "main.hpp"
#include "logger.hpp"
#include <iostream>
#include <ctime>

int main(int argc, char **argv) {
    config.workingDirectory = std::filesystem::path(argv[0]).parent_path();

    Logger::log("Opening configuration file");
    FILE *config_file = fopen("config.json", "r");
    if (config_file == NULL) {
        Logger::log("Failed to find config.json in the working directory. Please ensure your running this tool in the MyLinuxScripts directory.");
        return 1;
    }
    
    config.json = nlohmann::json::parse(config_file);

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        // settings
        if (arg == "--verbose" || arg == "-v") {
            config.verbose = true;
            Logger::debug("Verbose mode enabled.");
            Logger::debug(config.workingDirectory);
            continue;
        }
        if (arg == "--help" || arg == "-h") {
help:
            std::cout << "Commands:\n"
                      << "  pull - Pulls all dotfiles specified in config to files directory.\n"
                      << "  push - Overrites all dotfiles with specified ones in files directory.\n"
                      << "  compile - Compiles utility projects (like waybar_hider).\n"
                      << "  install - Installs packages listed in config (Including from AUR).\n"
                      << "  sync - Syncs local repo with online Github repository.\n"
                      << "  plugins - Installs Hyprland plugins.\n"
                      << "args : --verbose\n";
            break;
        }

        // tasks
        if (config.mode != 0)       continue;
        if (arg == "pull")          config.mode = operation::pull;
        else if (arg == "push")     config.mode = operation::push;
        else if (arg == "compile")  config.mode = operation::compile;
        else if (arg == "install")  config.mode = operation::install;
        else if (arg == "sync")     config.mode = operation::sync;
        else if (arg == "plugins")  config.mode = operation::plugins;
        else                        goto help;
    }

    switch (config.mode) {
        case 0:
            Logger::debug("Nothing to do :("); 
            break;
        case operation::pull: 
            pull_targets_from_os();
            break;
        case operation::push:
            push_targets_to_os();
            break;
        case operation::compile: 
            compile_utilies();
            break;
        case operation::install:
            install_packages();
            break;
        case operation::sync: 
            sync_git();
            break;
        case operation::plugins:
            install_hypr_plugins();
            break;
    }

    Logger::log("Completed tasks!");
}

// todo: combine pull and push to reduce the amount of code
void pull_targets_from_os() {
    auto targets = config.json["targets"].get<std::vector<std::string>>(); 
    filesystem::path relative_path = config.workingDirectory.string() + "/files/";

    if (filesystem::is_directory(relative_path)) {
        if(!ask_user_for_confirmation("Delete \"" + relative_path.string() + "\" directory? [y/n]")) {
            Logger::log("Aborting...");
            return;
        }
        Logger::log("Deleting directory");
        filesystem::remove_all(relative_path);
    }

    filesystem::create_directory(relative_path);
    for (string path_str : targets) {
        Logger::debug("Copying " + path_str);

        filesystem::path path(path_str);
        filesystem::path new_path(relative_path.string() + path.filename().string());
        filesystem::copy(path, new_path, filesystem::copy_options::recursive);
    }
}

void push_targets_to_os() {
    Logger::log("This will overwrite all files & directories specified in targets. Are you sure? [y/n]");
    char input;
    std::cin >> input;
    if (input != 'y' && input != 'Y') {
        Logger::log("Coward, aborting");
        return;
    }

    auto targets = config.json["targets"].get<std::vector<std::string>>(); 
    filesystem::path relative_path = config.workingDirectory.string() + "/files/";

    if (!filesystem::is_directory(relative_path)) {
        Logger::log("There are no files to push.... Aborting idiot.");
        return;
    }

    for (string path_str : targets) {
        if (path_str.empty()) continue;
        Logger::debug("Copying " + path_str);

        filesystem::path path(path_str);
        filesystem::path relative_file_path(relative_path.string() + path.filename().string());
        system(string("sudo cp -fr " + relative_file_path.string() + " " + path.parent_path().string()).c_str());
    }
    Logger::debug("Reloading hyprland.");
    system("hyprctl reload");
}

void compile_utilies() {
    Logger::log("Compiling utilies...");

    filesystem::path output_directory(config.workingDirectory.string() + "/scripts");
    if (filesystem::is_directory(output_directory)) {
        if (!ask_user_for_confirmation("Are you sure you wish to delete " + output_directory.string() + " [y/n]"))
            return;
        filesystem::remove_all(output_directory);
    }
    filesystem::create_directory(output_directory);

    auto utils = config.json["compile"];
    for (auto util : utils) {
        filesystem::path path(util["target"]);
        string tool = util["tool"];
        Logger::debug("Compiling " + path.string());

        string flags;
        for (string flag : util["flags"]) {
            Logger::debug("flag " + flag);
            flags.append(" " + flag);
        }

        string command = string(tool + " " + path.string() + " -o " + output_directory.string() + "/" + path.parent_path().filename().string() + flags);
        Logger::debug(command);
        system(command.c_str());
    }
}

void install_packages() {
    bool yay_not_installed = run_command("yay -h") == "bash: yay: command not found";
    if (yay_not_installed) {
        Logger::log("Please install yay.");
        return;
    }

    Logger::debug("Updating package list.");

    system("sudo pacman -Sy");

    auto packages = config.json["packages"];
    int installed_package_count = 0;
    for (string package : packages) {
        Logger::debug("Checking if " + package + " is installed");

        if (!run_command("pacman -Q " + package).empty() && 
             run_command("pacman -Qu " + package).empty()) {
                Logger::debug(package + " is already up to date!");
                continue;
        }

        bool is_aur_package = run_command("pacman -Si " + package).empty();
        if (is_aur_package) {
            Logger::log(package + " downloading from AUR via Yay");
            run_command("yay -S " + package + " --noconfirm");
            goto finished;
        }


        Logger::log("Installing " + package);
        system(string("sudo pacman -S " + package + " --noconfirm").c_str());

finished:
        installed_package_count++;
        Logger::log("Installed " + package);
    }
    Logger::log(installed_package_count == 0 ? "No packages installed" : ("Installed " + to_string(installed_package_count) + " package(s)!"));

    if (ask_user_for_confirmation("Installing Hyprland plugins aswell? [y/n]"))
        install_hypr_plugins();
}

void sync_git() {
    Logger::debug("Pulling files...");
    pull_targets_from_os();

    Logger::log("Git commit title:");
    string input_str;
    std::cin >> input_str;

    if (input_str.empty()) {
        time_t now = time(nullptr);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));
        input_str = string(buffer);
    }
    system(string("git add --all && git commit -m \""+ input_str + "\"").c_str());
    system("git pull && git push");
}

void install_hypr_plugins() {
    system("hyprpm update");
 
    auto plugins = config.json["hypr-plugins"];
    for (string plugin : plugins) {
        int last_slash = plugin.find_last_of('/');
        string plugin_name = plugin.substr(last_slash + 1, plugin.length() - last_slash);
        Logger::debug("Adding " + plugin_name);
        system(string("hyprpm add " + plugin + " && hyprpm enable " + plugin_name).c_str()); // if already installed it will do nothing except bitch at the user
    }
}

bool ask_user_for_confirmation(string query) {
    Logger::log(query);
    char input;
    std::cin >> input;
    if (input != 'y' && input != 'Y') {
        Logger::log("Aborting");
        return false;
    }
    return true;
}

std::string run_command(std::string cmd) {
    char buffer[128];
    string output;
    FILE* pipe = popen(cmd.c_str(), "r");
    while (fgets(buffer, sizeof buffer, pipe) != NULL) 
        output += buffer;
    return output;
}

void Logger::log(string txt) {
    std::cout << txt << "\n";
}

void Logger::debug(string txt) {
    if (config.verbose) 
        std::cout << "[DEBUG]: " << txt << "\n";
}