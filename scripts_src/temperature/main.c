#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

int main() {
    char *path = "/sys/class/thermal/thermal_zone2/temp";

    FILE* file;
    int temperature;
    bool critical;
loop:    
    
    file = fopen(path, "r");
    if (file == NULL) {
        printf("{\"text\":\"bruh\"}");
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
