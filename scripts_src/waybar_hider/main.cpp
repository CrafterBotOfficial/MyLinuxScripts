#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "./lib/json.hpp"

// Function to get the number of windows in the current workspace
int get_window_count() {
    FILE* pipe = popen("hyprctl activeworkspace -j", "r");
    if (!pipe) {
        std::cerr << "Failed to run hyprctl" << std::endl;
        return -1;
    }
    char buffer[1024];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    pclose(pipe);
    try {
        auto json = nlohmann::json::parse(result);
        return json["windows"].get<int>();
    } catch (const std::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        return -1;
    }
}

int main() {
    sleep(2);
    
    // Get HYPRLAND_INSTANCE_SIGNATURE from environment
    const char* his = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
    if (!his) {
        std::cerr << "HYPRLAND_INSTANCE_SIGNATURE not set" << std::endl;
        return 1;
    }

    // Construct socket path using XDG_RUNTIME_DIR or fallback to /tmp
    const char* xdg_runtime_dir = std::getenv("XDG_RUNTIME_DIR");
    std::string base_dir = xdg_runtime_dir ? xdg_runtime_dir : "/tmp";
    std::string socket_path = base_dir + "/hypr/" + std::string(his) + "/.socket2.sock";

    // Create Unix domain socket
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    // Set up socket address
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    // Connect to the event socket
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to connect to socket at " << socket_path << std::endl;
        close(sockfd);
        return 1;
    }

    // Associate FILE* with socket for line-based reading
    FILE* fp = fdopen(sockfd, "r");
    if (!fp) {
        std::cerr << "Failed to open socket as file stream" << std::endl;
        close(sockfd);
        return 1;
    }


    // Handle initial state
    int initial_count = get_window_count();
    if (initial_count < 0) {
        std::cerr << "Failed to get initial window count" << std::endl;
        fclose(fp);
        close(sockfd);
        return 1;
    }

    bool waybar_hidden = false; // Assume Waybar starts visible
    if (initial_count == 0) {
        std::cout << "Initial hide";
        system("killall -SIGUSR1 waybar");
        waybar_hidden = true;
    }

    // Main event loop
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fp)) {
        std::string line(buffer);
        size_t pos = line.find(">>");
        if (pos != std::string::npos) {
            std::string event = line.substr(0, pos);
            // React to events that might change the window count
            if (event == "workspace" || event == "openwindow" || 
                event == "closewindow" || event == "movewindow") {
                int window_count = get_window_count();
                if (window_count < 0) continue;

                if (window_count == 0 && !waybar_hidden) {
                    std::cout << "Hidding bar";
                    system("killall -SIGUSR1 waybar");
                    waybar_hidden = true;    
                } else if(window_count > 0 && waybar_hidden) {
                    std::cout << "Showing bar";
                    system("killall -SIGUSR1 waybar");
                    waybar_hidden = false;
                }
            }
        }
    }

    // Cleanup
    fclose(fp);
    close(sockfd);
    return 0;
}