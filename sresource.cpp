#include <iostream>
#include <stdexcept> // system command execute
#include <regex> // regular expression
#include <string>
#include <vector>
#include <fstream>
#include <unistd.h>
#include "other_file.h"
using namespace std;

struct resource {
    int mem;
    int cpus;
    bool gpu_full;
    vector<string> gres;
};

char* get_allocTres() {
    char* AllocTres = new char[1024];
    string command = "scontrol show node dynomics | grep AllocTRES";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe)
        throw runtime_error("Failed to open command.");
    if (fgets(AllocTres, 1024, pipe) == NULL)
        throw runtime_error("No information\n");
    return AllocTres;
}

char* get_confTres() {
    char* ConfTres = new char[1024];
    string command = "scontrol show node dynomics | grep CfgTRES";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe)
        throw runtime_error("Failed to open command.");
    if (fgets(ConfTres, 1024, pipe) == NULL)
        throw runtime_error("No information\n");
    return ConfTres;
}

int get_idle_mem(string ConfTres_str, string AllocTres_str) {
    int ConfmemCount, AllocmemCount;
    size_t ConfmemPos = ConfTres_str.find("mem=");
    if (ConfmemPos != string::npos) {
        string memSubstring = ConfTres_str.substr(ConfmemPos + 4);
        size_t commaPos = memSubstring.find(",");
        if (commaPos != string::npos) {
            string ConfmemCountString = memSubstring.substr(0, commaPos);
            istringstream(ConfmemCountString) >> ConfmemCount;
        }
    }
    size_t AllocmemPos = AllocTres_str.find("mem=");
    if (AllocmemPos != string::npos) {
        string memSubstring = AllocTres_str.substr(AllocmemPos + 4);
        size_t commaPos = memSubstring.find(",");
        if (commaPos != string::npos) {
            string AllocmemCountString = memSubstring.substr(0, commaPos);
            istringstream(AllocmemCountString) >> AllocmemCount;
        }
    }
    return ConfmemCount - AllocmemCount;
}

int get_idle_cpus(string ConfTres_str, string AllocTres_str) {
    int ConfcpusCount = 0, AlloccpusCount = 0;
    size_t ConfcpusPos = ConfTres_str.find("cpu=");
    if (ConfcpusPos != string::npos) {
        string cpusSubstring = ConfTres_str.substr(ConfcpusPos + 4);
        size_t commaPos = cpusSubstring.find(",");
        if (commaPos != string::npos) {
            string ConfcpusCountString = cpusSubstring.substr(0, commaPos);
            istringstream(ConfcpusCountString) >> ConfcpusCount;
        }
    }
    size_t AlloccpusPos = AllocTres_str.find("cpu=");
    if (AlloccpusPos != string::npos) {
        string cpusSubstring = AllocTres_str.substr(AlloccpusPos + 4);
        size_t commaPos = cpusSubstring.find(",");
        if (commaPos != string::npos) {
            string AlloccpusCountString = cpusSubstring.substr(0, commaPos);
            istringstream(AlloccpusCountString) >> AlloccpusCount;
        }
    }
    return ConfcpusCount - AlloccpusCount;
}

vector<string> get_idle_gpu(string ConfTres_str, string AllocTres_str) {
    vector<string> Conf_gpu, Alloc_gpu;
    regex pattern("gres\\/([a-zA-Z0-9:.=]+)");
    smatch match;
    string::const_iterator it(ConfTres_str.cbegin());
    string::const_iterator end(ConfTres_str.cend());
    while (regex_search(it, end, match, pattern)) {
        string gresString = match[1].str();
        Conf_gpu.push_back(gresString);
        // Advance the iterator to the next position
        it = match.suffix().first;
    }
    string::const_iterator it2(AllocTres_str.cbegin());
    string::const_iterator end2(AllocTres_str.cend());
    while (regex_search(it2, end2, match, pattern)) {
        string gresString = match[1].str();
        Alloc_gpu.push_back(gresString);
        it2 = match.suffix().first;
    }
    vector<string> idle_gpu;
    for (int i = 0; i < Conf_gpu.size(); i++) {
        bool flag = true;
        for (int j = 0; j < Alloc_gpu.size(); j++)
            if (Conf_gpu[i] == Alloc_gpu[j]) {
		flag = false;
                break;
	    }
        if (flag)
            idle_gpu.push_back(Conf_gpu[i]);
    }
    return idle_gpu;
}
void get_idle_source(resource idle_source) {
    // use anthoer function get confTres and allocTres
    char* AllocTres = new char[1024];
    char* ConfTres = new char[1024];
    AllocTres = get_allocTres();
    ConfTres = get_confTres();
    //chage the format of confTres and allocTres to string
    string AllocTres_str = AllocTres;
    string ConfTres_str = ConfTres;
    // get idle source
    idle_source.mem = get_idle_mem(ConfTres_str, AllocTres_str);
    cout << ConfTres << endl << AllocTres << endl;
    idle_source.cpus = get_idle_cpus(ConfTres_str, AllocTres_str);
    // get gpu information
    idle_source.gres = get_idle_gpu(ConfTres_str, AllocTres_str);
    delete[] AllocTres;
    delete[] ConfTres;
    cout << "Idle source: " << idle_source.mem << " " << idle_source.cpus << " " << endl;
    for (int i = 0; i < idle_source.gres.size(); i++)
        cout << idle_source.gres[i] << " ";
    cout << endl;
}
void get_resource() {
    resource idle_source;
    get_idle_source(idle_source);
}
