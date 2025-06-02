#include "config.hpp"

Config config;

void pull_targets_from_os();
void push_targets_to_os();
void compile_utilies();
void install_packages();
void install_hypr_plugins();
void sync_git();

bool ask_user_for_confirmation(std::string query);
std::string run_command(std::string cmd);