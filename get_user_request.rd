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
    struct user_request {
        vector<int> jobid;
        vector<int> jobid_run;
        vector<int> jobid_pending;
        int sumCPU;
        float summemory;
        int runCPU;
        float runmemory;
        int pendingCPU;
        float pendingmemory;
        vector<int> GPU;
        vector<int> runGPU;
        vector<int> pendingGPU;
        string username;
        string usergroup;
    };
    
    const vector<string> GPU_type = {"gres/gpu", "gres/gpu:7g.40gb", "gres/gpu:1g.5gb", "gres/gpu:2g.10gb", "gres/gpu:3g.20gb"};

    int get_job_status(int jobid, int i) {
        //sort the pending job and running job
        string command = "scontrol show jobid -dd " + to_string(jobid);
        FILE* fp = popen(command.c_str(), "r");
        if (fp == NULL) {
            cout << "Without job in squeue" << endl;
            exit(1);
        }
        char buf[8192];	
        while(fgets(buf, sizeof(buf), fp) != NULL) {
            string str = buf;
            if (str.find("JobState") != string::npos) {
              if (str.find("RUNNING") != string::npos)
		    return 1;
              else if (str.find("PENDING") != string::npos)
		    return 0;
		
            }
        }
	return 0;
    }

    void get_job_set(int i){
        char* command = new char[100];
        sprintf(command, "squeue -u %s -o \"%%i\" | sed -n '2, $p'", user_request_list[i].username.c_str());
        FILE* fp = popen(command, "r");
        char* buf = new char [8192];
        while(fgets(buf, sizeof(buf), fp) != NULL) {
            string str = buf;
            str = str.substr(0, str.length() - 1);
            user_request_list[i].jobid.push_back(atoi(str.c_str()));
	    int state = get_job_status(atoi(str.c_str()), i);
	    /*if(state==1)
		user_request_list[i].jobid_run.push_back(atoi(str.c_str()));
	    else
		user_request_list[i].jobid_pending.push_back(atoi(str.c_str()));*/
        }
	sort(user_request_list[i].jobid.begin(), user_request_list[i].jobid.end());
        pclose(fp);
    }

    void get_job_detail(int i) {
	    user_request_list[i].GPU.resize(5, 0);
        for(int j=0;j<user_request_list[i].jobid.size();j++) {
	        string command = "scontrol show jobid -dd " + to_string(user_request_list[i].jobid[j]);
            FILE* fp = popen(command.c_str(), "r");
	        if (fp == NULL) {
                cout << "Without job in squeue" << endl;
                exit(1);
            }
            int status = 0, place = 0; // pending is 1, running is 2
            for(int k=0;k<user_request_list[i].jobid_pending.size();k++) {
                if(user_request_list[i].jobid[j] == user_request_list[i].jobid_pending[k]) {
                    status = 1;
                    place = k;
                    break;
                }
            }
            for(int k=0;k<user_request_list[i].jobid_run.size();k++) {
                if(user_request_list[i].jobid[j] == user_request_list[i].jobid_run[k]) {
                    status = 2;
                    place = k;
                    break;
                }
            }
            char buf[8192];
            while(fgets(buf, sizeof(buf), fp) != NULL) {
		        string str = buf;
                if (str.find("NumCPUs") != string::npos) {
                    smatch match;
                    regex_search(str, match, regex("NumCPUs=(\\d+)"));
                    user_request_list[i].sumCPU += stoi(match[1].str());
                    if (status == 1)
                        user_request_list[i].pendingCPU += stoi(match[1].str());
                    else if (status == 2)
                        user_request_list[i].runCPU += stoi(match[1].str());
                }
	            if (str.find("mem=") != string::npos) {
                    regex pattern("mem=(\\d+)(M|G)");
                    smatch match;
                    regex_search(str, match, pattern);
                    if (match[2].str() == "M") {
                        user_request_list[i].summemory += stoi(match[1].str()) / 1024.0;
                        if (status == 1)
                            user_request_list[i].pendingmemory += stoi(match[1].str()) / 1024.0;
                        else if (status == 2)
                            user_request_list[i].runmemory += stoi(match[1].str()) / 1024.0;
                    } else {
                        user_request_list[i].summemory = stoi(match[1].str());
                        if (status == 1)
                            user_request_list[i].pendingmemory += stoi(match[1].str());
                        else if (status == 2)
                            user_request_list[i].runmemory += stoi(match[1].str());
                    }
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

                    while ((pos = gpu.find(delimiter)) != string::npos) {
                        token = gpu.substr(0, pos);
                        for (int x = 0; x < patterns.size(); x++) {
                            smatch match;
                            if (regex_search(token, match, patterns[x])) {
                                user_request_list[i].GPU[x] += stoi(match[1]);
                                if(status == 1)
                                    user_request_list[i].pendingGPU[x] += stoi(match[1]);
                                else if(status == 2)
                                    user_request_list[i].runGPU[x] += stoi(match[1]);
                                break;
                            }
                        }
                        gpu.erase(0, pos + delimiter.length());
                        for (int x = 0 ; x < patterns.size(); x++) {
                            smatch match;
                            if (regex_search(gpu, match, patterns[x])) {
                                user_request_list[i].GPU[x] += stoi(match[1]);
                                if(status == 1)
                                    user_request_list[i].pendingGPU[x] += stoi(match[1]);
                                else if(status == 2)
                                    user_request_list[i].runGPU[x] += stoi(match[1]);
                                break;
                            }
                        }
                    }
                }
	        }
	    }
    }

    void get_task_detail_user() {
        cout << "Total request resource: " << endl;
	cout << setw(15) << "Username" << setw(5) << "CPU" << setw(15) << "\t Memory(GB): "  << setw(10) << "\tAll GPU" << setw(10) << "\tGPU-full" << setw(10) << "\tGPU-1/7" << setw(10) << "\tGPU-3/7" << setw(10) << "\tGPU-4/7"<< endl;
        for(int i=0;i<user_request_list.size();i++) {
            get_job_set(i);
	}
	cout << "@";
	    for(int i=0;i<user_request_list.size();i++)
	        get_job_detail(i);
        
	    for(int i=0;i<user_request_list.size();i++){
            cout << setw(15) << user_request_list[i].username;
            cout  << setw(5) << user_request_list[i].sumCPU << setw(15) << user_request_list[i].summemory;
            for(int j=0;j< user_request_list[i].GPU.size();j++)
                cout << setw(10) << user_request_list[i].GPU[j] << "\t";
            cout << endl;
	    }
        /*cout << "Running tasks request resource: " << endl;
        cout << setw(15) << "Username" << setw(5) << "CPU" << setw(15) << "\t Memory(GB): "  << setw(10) << "\tAll GPU" << setw(10) << "\tGPU-full" << setw(10) << "\tGPU-1/7" << setw(10) << "\tGPU-3/7" << setw(10) << "\tGPU-4/7"<< endl;
        for(int i=0;i<user_request_list.size();i++) {
            cout << setw(15) << user_request_list[i].username;
            cout  << setw(5) << user_request_list[i].runCPU << setw(15) << user_request_list[i].runmemory;
            for(int j=0;j< user_request_list[i].runGPU.size();j++)
                cout << setw(10) << user_request_list[i].runGPU[j] << "\t";
            cout << endl;
        }
        cout << "Pending tasks request resource: " << endl;
        cout << setw(15) << "Username" << setw(5) << "CPU" << setw(15) << "\t Memory(GB): "  << setw(10) << "\tAll GPU" << setw(10) << "\tGPU-full" << setw(10) << "\tGPU-1/7" << setw(10) << "\tGPU-3/7" << setw(10) << "\tGPU-4/7"<< endl;
        for(int i=0;i<user_request_list.size();i++) {
            cout << setw(15) << user_request_list[i].username;
            cout  << setw(5) << user_request_list[i].pendingCPU << setw(15) << user_request_list[i].pendingmemory;
            for(int j=0;j< user_request_list[i].pendingGPU.size();j++)
                cout << setw(10) << user_request_list[i].pendingGPU[j] << "\t";
            cout << endl;
        }*/
    }

public:
    vector<user_request> user_request_list;
	
    void get_info() {
        FILE* fp = popen("squeue -o \"%%u\" | sed -n '2, $p'", "r");
        if (fp == NULL) {
            cout << "Without job in squeue" << endl;
            exit(1);
        }
        char buf[8192];
        vector<string> user_list;
        while (fgets(buf, sizeof(buf), fp) != NULL) {
            string str = buf;
            str = str.substr(0, str.length() - 1);
            user_list.push_back(str);
        }
	    pclose(fp);
	    sort(user_list.begin(), user_list.end());
        user_list.erase(unique(user_list.begin(), user_list.end()), user_list.end());
	    for(int i = 0; i < user_list.size(); i++) {
            user_request u;
            u.username = user_list[i];
	        u.summemory = 0;
	        u.sumCPU = 0;
            user_request_list.push_back(u);
        }
	    get_task_detail_user();
    }
};

void get_user_resource(int argc, char* argv[]) {
    UserRequest obj;
    obj.get_info();
}

