#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <regex>
#include <cstring>
#include "get_user_request.h"
using namespace std;

class UserRequest {
private:
    struct user_request {
        vector<int> jobid;
        int sumCPU;
        float summemory;
        vector<int> GPU;
        string username;
        string usergroup;
    };
    const vector<string> GPU_type = {"gres/gpu", "gres/gpu:7g.40gb", "gres/gpu:1g.5gb", "gres/gpu:2g.10gb", "gres/gpu:3g.20gb"};

    void get_job_set(int i){
        char* command = new char[100];
        sprintf(command, "squeue -u %s -o \"%%i\" | sed -n '2, $p'", user_request_list[i].username.c_str());
        FILE* fp = popen(command, "r");
        char* buf = new char [8192];
        while(fgets(buf, sizeof(buf), fp) != NULL) {
            string str = buf;
            str = str.substr(0, str.length() - 1);
            user_request_list[i].jobid.push_back(atoi(str.c_str()));
        }
	sort(user_request_list[i].jobid.begin(), user_request_list[i].jobid.end());
    }

    void get_job_detail(int i) {
        for(int j=0;j<user_request_list[i].jobid.size();j++) {
            char* command = new char[100];
            sprintf(command, "scontrol show job %d", user_request_list[i].jobid[j]);
            FILE* fp = popen(command, "r");
	    if (fp == NULL)
            {
                cout << "Without job in squeue" << endl;
                exit(1);
            }
            char buf[8192];
	    
	    vector<regex> patterns = {
                        regex("gres/gpu=(\\d+)"),
                        regex("gres/gpu:7g\\.40gb=(\\d+)"),
                        regex("gres/gpu:1g\\.5gb=(\\d+)"),
                        regex("gres/gpu:2g\\.10gb=(\\d+)"),
                        regex("gres/gpu:3g\\.20gb=(\\d+)")};
	    user_request_list[i].GPU.resize(patterns.size(), 0);

            while(fgets(buf, sizeof(buf), fp) != NULL) {
		string str = buf;
                if (str.find("NumCPUs") != string::npos) {
                    smatch match;
                    regex_search(str, match, regex("NumCPUs=(\\d+)"));
                    user_request_list[i].sumCPU += stoi(match[1].str());
                }
	        if (str.find("mem=") != string::npos) {
                        regex pattern("mem=(\\d+)(M|G)");
                        smatch match;
                        regex_search(str, match, pattern);
                        if (match[2].str() == "M")
                            user_request_list[i].summemory += stoi(match[1].str()) / 1024.0;
                        else
                            user_request_list[i].summemory = stoi(match[1].str());
                }
		if (str.find("gres/gpu") != string::npos)
                {
                    string gpu = str.substr(str.find("gres/gpu"), str.length() - str.find("gres/gpu") - 1);
                    string delimiter = ",";
		    size_t pos = 0;
                    string token;
                    while ((pos = gpu.find(",")) != string::npos)
                    {
                        token = gpu.substr(0, pos);
                        for (int x = 0; x < patterns.size(); x++)
                        {
                            smatch match;
                            if (regex_search(token, match, patterns[x]))
                            {
                                user_request_list[i].GPU[x] = stoi(match[1]);
                                break;
                            }
                        }
                        gpu.erase(0, pos + delimiter.length());
                        for (int x = 0; x < patterns.size(); x++)
                        {
                            smatch match;
                            if (regex_search(token, match, patterns[x]))
                            {
                                user_request_list[i].GPU[x] = stoi(match[1]);
                                break;
                            }
                        }

                    }
                }

	    }
	}
    }
    void get_task_detail_user() {
        for(int i=0;i<user_request_list.size();i++)
            get_job_set(i);

	for(int i=0;i<user_request_list.size();i++)
	    get_job_detail(i);
	for(int i=0;i<user_request_list.size();i++){
            cout << user_request_list[i].username << "\t Job: ";
            for(int j=0;j<user_request_list[i].jobid.size();j++)
                cout << user_request_list[i].jobid[j] << " ";
            cout << "\t CPU: " << user_request_list[i].sumCPU << "\t Memory: " << user_request_list[i].summemory << "\t GPU: ";
            for(int j=0;j<user_request_list[i].GPU.size();j++)
                cout << user_request_list[i].GPU[j] << " ";
	    cout << endl;
	}
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
        //remove the duplicate username

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

