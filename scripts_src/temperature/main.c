#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    char *path = "/sys/class/thermal/thermal_zone2/temp"; // laptop
    if (access(path, F_OK) != 0) path = "/sys/class/hwmon/hwmon2/temp1_input"; // desktop

    FILE* file;
    int temperature;
    bool critical;
loop:
    
    file = fopen(path, "r");
    if (file == NULL) {
        system("hyprctl notify -1 10000 \"rgb(f90409)\" \"Couldn't find path for thermal display waybar.\"");
        return 1;
    }
    fscanf(file, "%d", &temperature);
    temperature = temperature / 1000;

    critical = temperature >= 80;
    char *icon = critical ? "" : "";
    char *class = critical ? "hot" : "";
    printf("{\"text\":\"%d°C %s\",\"class\":\"%s\"}\n", temperature, icon, class);
    fflush(stdout);

    fclose(file);
    sleep(1);
    goto loop;
    return 0;
}
