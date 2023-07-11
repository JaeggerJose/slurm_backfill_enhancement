#include <iostream>
#include <stdexcept> // system command execute
#include <regex> // regular expression
#include <string>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <iomanip>

#include "node_resource.h"
using namespace std;

struct resource {
    float mem;
    int cpus;
    bool gpu_full;
    vector<int> gres;
    int gpu_num;
};

resource alloc_resource;
resource conf_resource;

char* get_allocTres() {
    char* AllocTres = new char[1024];
    string command = "scontrol show node dynomics | grep AllocTRES";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe)
        throw runtime_error("Failed to open command.");
    if (fgets(AllocTres, 1024, pipe) == NULL)
        throw runtime_error("No information1\n");
    return AllocTres;
}

char* get_confTres() {
    char* ConfTres = new char[1024];
    string command = "scontrol show node dynomics | grep CfgTRES";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe)
        throw runtime_error("Failed to open command.");
    if (fgets(ConfTres, 1024, pipe) == NULL)
        throw runtime_error("No information2\n");
    return ConfTres;
}
float get_idle_mem(string ConfTres_str, string AllocTres_str) {
    regex pattern("mem=(\\d+)(M|G)");
    int ConfmemCount, AllocmemCount;
    size_t ConfmemPos = ConfTres_str.find("mem=");
    if (ConfmemPos != string::npos) {
	    smatch match;
        regex_search(ConfTres_str, match, pattern);
        if (match[2].str() == "M")
            conf_resource.mem = stof(match[1].str()) / 1024.0;
        else
            conf_resource.mem = stof(match[1].str());
    }
    size_t AllocmemPos = AllocTres_str.find("mem=");
    if (AllocmemPos != string::npos) {
        smatch match;
	    regex_search(AllocTres_str, match, pattern);
        if (match[2].str() == "M")
           alloc_resource.mem = stof(match[1].str()) / 1024.0;
        else
           alloc_resource.mem = stof(match[1].str());

    }
    return conf_resource.mem - alloc_resource.mem;
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
    conf_resource.cpus = ConfcpusCount;
    alloc_resource.cpus = AlloccpusCount;
    return ConfcpusCount - AlloccpusCount;
}

vector<int> get_idle_gpu(string ConfTres_str, string AllocTres_str) {
    vector<int> idle_gpu;
    if (ConfTres_str.find("gres/gpu") != string::npos) {
        string gpu = ConfTres_str.substr(ConfTres_str.find("gres/gpu"), ConfTres_str.length() - ConfTres_str.find("gres/gpu") - 1);
        string delimiter = ",";
        size_t pos = 0;
        string token;
        vector<regex> patterns = {
            regex("gres/gpu=(\\d+)"),
            regex("gres/gpu:7g\\.40gb=(\\d+)"),
            regex("gres/gpu:1g\\.5gb=(\\d+)"),
            regex("gres/gpu:2g\\.10gb=(\\d+)"),
            regex("gres/gpu:3g\\.20gb=(\\d+)")
        };
        conf_resource.gres.resize(patterns.size(),0);
        while ((pos = gpu.find(delimiter)) != std::string::npos) {
            token = gpu.substr(0, pos);
            for (int i = 0; i < patterns.size(); i++) {
                smatch match;
                if (std::regex_search(token, match, patterns[i])) {
                    conf_resource.gres[i] = stoi(match[1]);
                    break;
                }
            }
            gpu.erase(0, pos + delimiter.length());
            for (int i = 0; i < patterns.size(); i++) {
                smatch match;
                if (regex_search(gpu, match, patterns[i])) {
                    conf_resource.gres[i] = stoi(match[1]);
                    break;
                }
            }
        }
    }

    if (AllocTres_str.find("gres/gpu") != string::npos) {
        string gpu = AllocTres_str.substr(AllocTres_str.find("gres/gpu"), AllocTres_str.length() - AllocTres_str.find("gres/gpu") - 1);
        string delimiter = ",";
        size_t pos = 0;
        string token;
        vector<regex> patterns = {
            regex("gres/gpu=(\\d+)"),
            regex("gres/gpu:7g\\.40gb=(\\d+)"),
            regex("gres/gpu:1g\\.5gb=(\\d+)"),
            regex("gres/gpu:2g\\.10gb=(\\d+)"),
            regex("gres/gpu:3g\\.20gb=(\\d+)")
        };
        alloc_resource.gres.resize(patterns.size(),0);
        while ((pos = gpu.find(delimiter)) != std::string::npos) {
            token = gpu.substr(0, pos);
            for (int i = 0; i < patterns.size(); i++) {
                smatch match;
                if (std::regex_search(token, match, patterns[i])) {
                    alloc_resource.gres[i] = stoi(match[1]);
                    break;
                }
            }
            gpu.erase(0, pos + delimiter.length());
            for (int i = 0; i < patterns.size(); i++) {
                smatch match;
                if (regex_search(gpu, match, patterns[i])) {
                    alloc_resource.gres[i] = stoi(match[1]);
                    break;
                }
            }
        }
    }
    idle_gpu.resize(5,0);
    for(size_t i=0;i<alloc_resource.gres.size();i++)
	idle_gpu[i] = conf_resource.gres[i] - alloc_resource.gres[i];
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
    idle_source.cpus = get_idle_cpus(ConfTres_str, AllocTres_str);
    // get gpu information
    idle_source.gres = get_idle_gpu(ConfTres_str, AllocTres_str);
    delete[] AllocTres;
    delete[] ConfTres;
    cout << setw(20) << "Resource\t" << setw(12)  << "Memory(GB)" <<  setw(5) << "CPU" << setw(10) << "\tAll GPU" << setw(10) << "\tGPU-full" << setw(10) << "\tGPU-1/7" << setw(10) << "\tGPU-4/7" << setw(10) << "\tGPU-3/7"<< endl;
    cout << setw(20) << "Configure resource:\t" << setw(12) << (conf_resource.mem)  << setw(5) << conf_resource.cpus;
    for (int i = 0; i < conf_resource.gres.size(); i++)
        cout << setw(10) << conf_resource.gres[i] << "\t";
    cout << endl;
    cout << setw(20) << "Allocated resource:\t" <<  setw(12) << (alloc_resource.mem) << setw(5) << alloc_resource.cpus;
    for (int i = 0; i < alloc_resource.gres.size(); i++)
        cout << setw(10) << alloc_resource.gres[i] << "\t";
    cout << endl;
    cout <<setw(20) << "Idle resource:\t" << setw(12) << (idle_source.mem)  << setw(5) << idle_source.cpus;
    for (int i = 0; i < idle_source.gres.size(); i++)
        cout << setw(10) << idle_source.gres[i] << "\t";
    cout << endl;
}
void get_resource() {
    resource idle_source;
    get_idle_source(idle_source);
}

