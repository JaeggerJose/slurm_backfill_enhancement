#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <regex>
#include <cstring>
#include <iomanip>

#include "get_user_request.h"
using namespace std;

class UserRequest {
private:
    const vector<string> GPU_type = {"gres/gpu", "gres/gpu:7g.40gb", "gres/gpu:1g.5gb", "gres/gpu:2g.10gb", "gres/gpu:3g.20gb"};
    struct job {
        int jobid;
        bool status; // 1 is pending, 2 is running
        int CPU;
        float memory;
        string username;
        vector<int> GPU;
    };
    struct info {
        int sumCPU;
        float summemory;
        string username;
        vector<int> GPU;
    };
    void get_job_status(job& job) {
        string cmd = "squeue -o \"%T\" -j " + to_string(job.jobid) + " | sed -n '2, $p'";
        FILE* fp = popen(cmd.c_str(), "r");
        char buf[1024];
        fgets(buf, 1024, fp);
        pclose(fp);
        if (strcmp(buf, "RUNNING\n") == 0)
            job.status = 1;
        else
            job.status = 0;
    }
    void get_job_detail(job& job) {
        // get username
        string cmd = "squeue -o \"%u\" -j " + to_string(job.jobid) + " | sed -n '2p'";
        string str;
        FILE* fp = popen(cmd.c_str(), "r");
        char buf[1024];
        fgets(buf, sizeof(buf), fp);
        pclose(fp);
        buf[strlen(buf) - 1] = '\0';
        job.username = buf;
        // get CPU
        cmd = "scontrol show job " + to_string(job.jobid) + " | grep NumCPUs";
        fp = popen(cmd.c_str(), "r");
        fgets(buf, sizeof(buf), fp);
        str = buf;
        pclose(fp);
        smatch result;
        regex_search(str, result, regex("NumCPUs=(\\d+)"));
        job.CPU = stoi(result[1]);
       // get memory
        cmd = "scontrol show job " + to_string(job.jobid) + " | grep mem";
        fp = popen(cmd.c_str(), "r");
        fgets(buf, sizeof(buf), fp);
        str = buf;
        pclose(fp);
        regex_search(str, result, regex("mem=(\\d+)(M|G)"));
        if (result[2] == "M")
            job.memory = stof(result[1]) / 1024.0;
        else
            job.memory = stof(result[1]);
        // get GPU
        cmd = "scontrol show job " + to_string(job.jobid) + " | grep gres ";
        fp = popen(cmd.c_str(), "r");
        fgets(buf, sizeof(buf), fp);
        str = buf;
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
         while ((pos = gpu.find(delimiter)) != std::string::npos) {
            token = gpu.substr(0, pos);
            for (int i = 0; i < patterns.size(); i++) {
                smatch match;
                if (regex_search(token, match, patterns[i])) {
                     job.GPU[i] = stoi(match[1]);
                     break;
                 }
             }
             gpu.erase(0, pos + delimiter.length());
             for (int i = 0; i < patterns.size(); i++) {
                 smatch match;
                 if (regex_search(gpu, match, patterns[i])) {
                      job.GPU[i] = stoi(match[1]);
                      break;
                  }
              }
         }
        }
    }

    void summary_job(vector<job> joblist, bool isRun) {
        vector<info> infolist;
        // if same username add into one info
        for(int i=0;i<joblist.size();i++){
            bool flag = false;
            for(int j=0;j<infolist.size();j++){
                if(joblist[i].username == infolist[j].username){
                    flag = true;
                    infolist[j].sumCPU += joblist[i].CPU;
                    infolist[j].summemory += joblist[i].memory;
                    for(int k=0;k<5;k++){
                        infolist[j].GPU[k] += joblist[i].GPU[k];
                    }
                    break;
                }
            }
            if(!flag){
                info tmp;
                tmp.username = joblist[i].username;
                tmp.sumCPU = joblist[i].CPU;
                tmp.summemory = joblist[i].memory;
                tmp.GPU = joblist[i].GPU;
                infolist.push_back(tmp);
            }
        }
	if(isRun)
	    cout << endl << "Running Task Allocated Resource" << endl;
	else
	    cout << endl << "Pending Task Request Reqource" << endl;
	cout << setw(20) << "UserName" << setw(10) << "CPU" << setw(10) << "Memory(G)" << setw(10) << "GPU-ALL" << setw(10) << "GPU-full" << setw(10) << "GPU-1/7" << setw(10) << "GPU-3/7" << setw(10) << "GPU-4/7" << endl;
        // print info
        for(int i=0;i<infolist.size();i++){
            cout << setw(20) << infolist[i].username << setw(10) << infolist[i].sumCPU << setw(10) << infolist[i].summemory;
            for(int j=0;j<5;j++){
                cout  << setw(10) << infolist[i].GPU[j] ;
            }
	    cout << endl;
        }
    }

    void sort_job_type(vector<job>& joblist) {
        // sort by pending and running and add all with corresponding username
        vector<job> pending, running;
        for (int i = 0; i < joblist.size(); i++) {
            get_job_status(joblist[i]);
            if (joblist[i].status)
                running.push_back(joblist[i]);
            else
                pending.push_back(joblist[i]);
        }
        summary_job(pending, false);
        summary_job(running, true);
    }

    public:

    void get_info() {
        string cmd = "squeue -o \"%i\" -S \"U\" | sed -n '2, $p'";
        FILE* fp = popen(cmd.c_str(), "r");
        char buf[1024];
        vector<int> jobid;
        while (fgets(buf, 1024, fp) != NULL) {
            jobid.push_back(atoi(buf));
        }
        pclose(fp);
        vector<job> joblist(jobid.size());
        for(int i=0;i<jobid.size();i++){
            joblist[i].GPU.resize(5);
            for(int j=0;j<5;j++){
                joblist[i].GPU[j]=0;
            }
        }
        for (int i = 0; i < jobid.size(); i++) {
            joblist[i].jobid = jobid[i];
            get_job_status(joblist[i]);
            get_job_detail(joblist[i]);
        }
        sort_job_type(joblist);
    }
};

void get_user_resource(int argc, char* argv[]) {
    UserRequest obj;
    obj.get_info();
}

