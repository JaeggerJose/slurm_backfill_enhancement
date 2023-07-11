#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <regex>
#include <iomanip>
#include "sheighest.h"

using namespace std;
const regex error_pattern("slurm_load_jobs error: Invalid job id specified");

class Heighest {
    private:
        struct sheighest {
            int jobid;
            string partition;
            int priority;
            int CPU;
            float memory;
            vector<int> GPU;
            string username;
            string group;
            string start_time;
        };
        const vector<string> GPU_type = {"gres/gpu", "gres/gpu:7g.40gb", "gres/gpu:1g.5gb", "gres/gpu:2g.10gb", "gres/gpu:3g.20gb"};

        void get_priority(sheighest& h) {
            string cmd = "sprio --sort=-y -o \"%Y\" | sed -n '2p'";
            FILE* fp = popen(cmd.c_str(), "r");
            char buf[8192];
	    while(fgets(buf, sizeof(buf), fp) != NULL) {
                string str = buf;
		if(str.find("error")!=string::npos) {
		    cout << "error";
		    exit(1);
		}
                str = str.substr(0, str.length() - 1);
                h.priority = atoi(str.c_str());
            }
            pclose(fp);
        }

        void get_partition(sheighest& h) {
            string cmd = "sprio --sort=-y -o \"%r\" | sed -n '2p'";
            FILE* fp = popen(cmd.c_str(), "r");
            char buf[8192];
            while(fgets(buf, sizeof(buf), fp) != NULL) {
                string str = buf;
                str = str.substr(0, str.length() - 1);
                h.partition = str;
            }
            pclose(fp);
        }
        
        void get_cpu(sheighest& h) {
            string cmd = "scontrol show job -dd " + to_string(h.jobid) + " | grep cpu";
            FILE* fp = popen(cmd.c_str(), "r");
            char buf[8192];
            while(fgets(buf, sizeof(buf), fp) != NULL) {
                string str = buf;
		if(regex_search(str, error_pattern)) {
		    cout << "Error: Invalid job id specified" << endl;
		    exit(1);
		}

                smatch match; 
                regex_search(str, match, regex("cpu=(\\d+)"));
                h.CPU = stoi(match[1]);
            }
            pclose(fp);
        }

        void get_memory(sheighest& h) {
            string cmd = "scontrol show job -dd " + to_string(h.jobid) + " | grep mem";
            FILE* fp = popen(cmd.c_str(), "r");
            char buf[8192];
            while(fgets(buf, sizeof(buf), fp) != NULL) {
                string str = buf;
                str.find("mem=");
		smatch match;
                regex_search(str, match, regex("mem=(\\d+)(M|G)"));
                    if (match[2].str() == "M")
                        h.memory = stoi(match[1].str()) / 1024.0;
                    else
                        h.memory = stoi(match[1].str());
            }
            pclose(fp);
        }

        void get_gpu(sheighest& h) {
            string cmd = "scontrol show job -dd " + to_string(h.jobid) + " | grep gres/gpu";
            FILE* fp = popen(cmd.c_str(), "r");
            char buf[8192];
            while(fgets(buf, sizeof(buf), fp) != NULL) {
                string str = buf;
                if (str.find("gres/gpu") != string::npos) {
                    string gpu = str.substr(str.find("gres/gpu"), str.length() - str.find("gres/gpu") - 1);
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
                    h.GPU.resize(patterns.size());
                    while ((pos = gpu.find(delimiter)) != std::string::npos) {
                        token = gpu.substr(0, pos);
                        for (int i = 0; i < patterns.size(); i++) {
                            smatch match;
                            if (regex_search(token, match, patterns[i])) {
                                h.GPU[i] = std::stoi(match[1]);
                                break;
                            }
                        }
                        gpu.erase(0, pos + delimiter.length());
                        for (int i = 0; i < patterns.size(); i++) {
                            smatch match;
                            if (regex_search(gpu, match, patterns[i])) {
                                h.GPU[i] = stoi(match[1]);
                                break;
                            }
                        }
                    }
                }
            }
            pclose(fp);
        }

        void get_user(sheighest& h) {
            string cmd = "scontrol show job -dd " + to_string(h.jobid) + " | grep UserId";
            FILE* fp = popen(cmd.c_str(), "r");
            char buf[8192];
            while(fgets(buf, sizeof(buf), fp) != NULL) {
                string str = buf;
                str.find("UserId=");
                smatch match;
                regex_search(str, match, regex("UserId=(\\w+)"));
                h.username = match[1];
            }
            pclose(fp);
        }

        void get_group(sheighest& h) {
            string cmd = "scontrol show job -dd " + to_string(h.jobid) + " | grep GroupId";
            FILE* fp = popen(cmd.c_str(), "r");
            char buf[8192];
            while(fgets(buf, sizeof(buf), fp) != NULL) {
                string str = buf;
                str.find("GroupId=");
                smatch match;
                regex_search(str, match, regex("GroupId=(\\w+)"));
                h.group = match[1];
            }
            pclose(fp);
        }

        void get_start_time(sheighest& h) {
            string cmd = "scontrol show job -dd " + to_string(h.jobid) + " | grep StartTime";
            FILE* fp = popen(cmd.c_str(), "r");
            char buf[8192];
            while(fgets(buf, sizeof(buf), fp) != NULL) {
                string str = buf;
                smatch match;
                regex_search(str, match, regex("StartTime=(\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2})"));
                h.start_time = match[1];
            }
            pclose(fp);
        }

        void print_heighest(sheighest h) {
            cout << "JobID: " << h.jobid << endl;
	    cout << "User: " << h.username << "\tGroup: " << h.group << endl;
            cout << "Start Time: " << h.start_time << endl;
            cout << "Priority: " << h.priority << endl;
            cout << "Partition: " << h.partition << endl;
            cout << "CPU: " << h.CPU << endl;
            cout << "Memory(GB): " << h.memory << endl;
            cout << "GPU Type:       ";
            for (int i = 0; i < GPU_type.size();i++) {
                if(i != 0)
                    cout << setw(20) << GPU_type[i] << " ";
                else
                    cout << setw(10) << GPU_type[i] << " ";
	    }
            cout << endl << "Request GPU: ";
            for (int i = 0; i < h.GPU.size(); i++) {
                if(i != 0)
                    cout << setw(20) << h.GPU[i] << " ";
                else
                    cout << setw(10) << h.GPU[i] << " ";
	    }
	    cout << endl;
	}
    public:

    void get_heighest() {
        sheighest h;
        string cmd = "sprio --sort=-y -o \"%i\" | sed -n '2p'";
        FILE* fp = popen(cmd.c_str(), "r");
        char buf[8192];
        while(fgets(buf, sizeof(buf), fp) != NULL) {
            string str = buf;
            str = str.substr(0, str.length() - 1);
            h.jobid = atoi(str.c_str());
	}
        get_priority(h);
        get_partition(h);
        get_cpu(h);
        get_memory(h);
        get_gpu(h);
        get_user(h);
        get_group(h);
        get_start_time(h);

        print_heighest(h);
    }
};

void get_high_priority(int argc, char* argv[]) {
    Heighest H;
    H.get_heighest();
}
