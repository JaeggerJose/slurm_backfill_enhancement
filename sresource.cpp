#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <regex>
#include <iomanip>
#include "node_resource.h"
#include "get_user_request.h"
#include "sheighest.h"
#include "salgorithm.h"
using namespace std;

class Resource {
    private:
        struct sresource {
            string username;
            int jobid;
            string jobstatus;
            int CPU;
            float memory;
            vector<int> GPU; 
        };
	    const vector<string> GPU_type = {"gres/gpu", "gres/gpu:7g.40gb", "gres/gpu:1g.5gb", "gres/gpu:2g.10gb", "gres/gpu:3g.20gb"};
        vector<sresource> get_resource(vector<int> jobid) {
	        vector<sresource> sresource_list(jobid.size());
            for(int i=0;i<jobid.size();i++) {
                string cmd = "scontrol show jobid -dd " + to_string(jobid[i]);
                FILE* fp = popen(cmd.c_str(), "r");
                if (fp == NULL) {
                    cout << "Without job in squeue" << endl;
                    exit(1);
                }
                char buf[8192];
                sresource s;
                while (fgets(buf, sizeof(buf), fp) != NULL) {
                    string str = buf;
                    if (str.find("UserId") != string::npos) {
			regex pattern("UserId=(\\w+)");
			smatch match;
			regex_search(str, match, pattern);
			s.username = match[1];
		    }
                    if (str.find("JobId") != string::npos)
                        s.jobid = atoi(str.substr(str.find("=") + 1, str.length() - str.find("=") - 2).c_str());
                    if (str.find("JobState") != string::npos)
                        s.jobstatus = str.substr(str.find("=") + 1, str.length() - str.find("=") - 2);
	 	            if (str.find("NumCPUs") != string::npos) {
                        smatch match; 
                        regex_search(str, match, regex("NumCPUs=(\\d+)"));
                        s.CPU = stoi(match[1]);
                    }
                    if (str.find("mem=") != string::npos) {
                        regex pattern("(\\d+(?:\\.\\d+)?)(M|G)");
                        smatch match;
                        regex_search(str, match, pattern);
			if(match.empty()) {
				s.memory = 0;
				continue;
			}
                        if (match[2].str() == "M")
                            s.memory = stof(match[1]) / 1024.0;
                        else
                            s.memory = stof(match[1]);
		            }
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
                        s.GPU.resize(patterns.size());
                        while ((pos = gpu.find(delimiter)) != std::string::npos) {
                            token = gpu.substr(0, pos);
                            for (int i = 0; i < patterns.size(); i++) {
                                std::smatch match;
                                if (std::regex_search(token, match, patterns[i])) {
                                    s.GPU[i] = std::stoi(match[1]);
                                    break;
                                }
                            }
                            gpu.erase(0, pos + delimiter.length());
			                for (int i = 0; i < patterns.size(); i++) {
                                smatch match;
                                if (regex_search(gpu, match, patterns[i])) {
                                    s.GPU[i] = stoi(match[1]);
                                    break;
                                }
                            }
                        }
                    }
                }
                pclose(fp);
                sresource_list[i] = s;
            }
            return sresource_list;       
        }
    public:
    vector<sresource> job_list;
    void get_all_squeue_resource() {
        FILE* fp = popen("squeue -o \"%A\" | sed -n '2,$p'", "r");
        if (fp == NULL) {
            cout << "Without job in squeue" << endl;
            exit(1);
        }
        vector<int> jobid;
        char buf[8192];
        while (fgets(buf, sizeof(buf), fp) != NULL)
            jobid.push_back(atoi(buf));
        pclose(fp);
        job_list = get_resource(jobid);
        // print the jobid
    }
    void print_sresource() {
        cout << setw(20) << "Username" << setw(8) << "JobID" << setw(60) << "JobStatus" << setw(6) << "CPU" << setw(12) << "Memory(GB)" << setw(25) << "GPU" << endl;
        for(int i=0;i<job_list.size();i++) {
            cout << setw(20) << job_list[i].username << setw(8) << job_list[i].jobid << setw(60) << job_list[i].jobstatus << setw(6) << job_list[i].CPU << setw(12) << job_list[i].memory << setw(25);
            for (int j = 0; j < job_list[i].GPU.size(); j++)
		    if(job_list[i].GPU[j]!=0)
                cout << GPU_type[j]<<": " << job_list[i].GPU[j] << "\t";
            cout << endl;
	    }
    }
    void sort_by_username() {
       sort(job_list.begin(), job_list.end(), [](sresource a, sresource b) {
            return a.username < b.username;
        });
        print_sresource();
    }
    void sort_by_jobid() {
        sort(job_list.begin(), job_list.end(), [](sresource a, sresource b) {
            return a.jobid < b.jobid;
        });
	    print_sresource();
    }
    void sort_by_jobstatus() {
        sort(job_list.begin(), job_list.end(), [](sresource a, sresource b) {
            return a.jobstatus < b.jobstatus;
        });
        print_sresource();
    }
    void sort_by_CPU() {
        sort(job_list.begin(), job_list.end(), [](sresource a, sresource b) {
            return a.CPU < b.CPU;
        });
        print_sresource();
    }
    void sort_by_memory() {
        sort(job_list.begin(), job_list.end(), [](sresource a, sresource b) {
            return a.memory < b.memory;
        });
        print_sresource();
    }
};
void get_help() {
    // print help list of sresource
    cout << "Usage: sresource [OPTION]..." << endl;
    cout << "Show the resource usage of the cluster" << endl;
    cout << "this tag is exclusive, you can only use one of them" << endl;
    cout << "  -n, --node is to show the resource usage of the cluster" << endl;
    cout << "  -u, --user is to show the resource usage of the user, who has task(s) in squeue" << endl;
    cout << "  -p, --priority is to show the resource usage of the high priority task" << endl;
    cout << "  -a, --algorithm is to show the algorithm of slurm" << endl;
    cout << "  -s, --sort is to sort the resource usage of queuing task" << endl;
    cout << "       for -s and --sort you can add these tag to sort the attribute" << endl;
    cout << "       (username, jobid, jobstatus, CPU, memory) e.g.: sresource -s jobid" << endl;
    cout << "       these sorting tag are also exclusive, you can only use one of them" << endl;
    cout << "  -h, --help display this help and exit" << endl;
    exit(1);
}

int main(int argc, char** argv) {
    Resource r;
    if(argv[1] && (!strcmp(argv[1], "-n") || !strcmp(argv[1], "--node"))) {
        get_resource();
	get_high_priority(argc, argv);
	    return 0;
    } else if(argv[1] && (!strcmp(argv[1], "-u") || !strcmp(argv[1], "--user"))) {
        get_user_resource(argc, argv);
	    return 0;
    } else if(argv[1] && (!strcmp(argv[1], "-p") || !strcmp(argv[1], "--priority"))) {
        get_high_priority(argc, argv);
        return 0;
    } else if(argv[1] && (!strcmp(argv[1], "-a") || !strcmp(argv[1], "--algorithm"))) {
        get_algorithm();
        return 0;
    } else if(argv[1] && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        get_help();
    }
    r.get_all_squeue_resource();
    if (argc >= 2 && (!strcmp(argv[1], "-s") || !strcmp(argv[1], "--sort")) && argv[2]) {
        if (strcmp(argv[2], "username") == 0)
            r.sort_by_username();
        if (strcmp(argv[2], "jobid") == 0)
            r.sort_by_jobid();
        if (strcmp(argv[2], "jobstatus") == 0)
            r.sort_by_jobstatus();
        if (strcmp(argv[2], "CPU") == 0)
            r.sort_by_CPU();
        if (strcmp(argv[2], "memory") == 0)
            r.sort_by_memory();
        return 0;
    }
    r.print_sresource();
    return 0;
}
