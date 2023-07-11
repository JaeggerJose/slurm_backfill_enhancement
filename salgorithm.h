#ifndef ALGORITHM_H
#define ALGORITHM_H
#include <iostream>
#include <fstream>
#include <string>
#include <regex>

void get_algorithm() {
    FILE* fp = popen("cat /etc/slurm-llnl/slurm.conf", "r");
    char buf[8192];
    while(fgets(buf, sizeof(buf), fp) != NULL) {
        std::string str = buf;
        if (str.find("SchedulerType") != std::string::npos) {
            std::smatch match;
            std::regex_search(str, match, std::regex("SchedulerType=(sched/builtin|sched/backfill)"));
            std::cout << "Now slurm algorithm is " << match[1] << "." << std::endl;
        }
    }
    pclose(fp);
}
#endif
