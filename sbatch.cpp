#include <iostream>
#include <stdexcept> // system command execute
#include <regex> // regular expression
#include <queue>
#include <vector>
#include <string>
#include <fstream>

using namespace std;

char* get_start_time(int job_id) {
    char* command = new char[256];
    sprintf(command, "squeue -j %d --format \"%%S\" | sed -n \"2p\"", job_id);
    string command_str =  command;
    delete[] command;
    FILE* pip = popen(command_str.c_str(), "r");
    if(!pip)
       throw runtime_error("Failed to open command."); 
    char* start_time = new char[256];
    if (fgets(start_time, 256, pip) == nullptr) {
        cout << "No job in the queue.\n";
        throw runtime_error("No job in the queue.\n"); 
    }
    return start_time;
}

char* get_high_priority_start_time() {
    // get all user jobs in squeue and get the highest priority job's start time
    string command = "sprio --sort=-y -o \"%.15i\" | sed -n \"2p\"";
    vector<char> job_id(16);
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        cout << "failed";
        throw runtime_error("Failed to open command.");
    }
    if (fgets(job_id.data(), 16, pipe) == NULL) {
        cout << "No job in the queue.\n";
        throw runtime_error("No job in the queue.\n");
    }
    string new_job_id = job_id.data();
    new_job_id.erase(0, new_job_id.find_first_not_of(' ')); // remove leading space
    return get_start_time(stoi(new_job_id));
}

char* clone_file(string jobName, string mem, string gres, string partition, string account, int nTasks, int cpusPerTask) {

}

void make_clone_script(string filename, string start_time) {
    ifstream scriptFile(filename);
    regex attributeRegex(R"(^#SBATCH\s+--(\w+)\s*=\s*(.*)$)");
    string line, jobName, mem, gres, partition, account;
    int nTasks = 0, cpusPerTask = 0;
    
    while (std::getline(scriptFile, line)) {
        smatch match;
        if (regex_match(line, match, attributeRegex)) {
            string attributeName = match[1];
            string attributeValue = match[2];

            if (attributeName == "ntasks") {
                nTasks = std::stoi(attributeValue);
            } else if (attributeName == "mem") {
                mem = attributeValue;
            } else if (attributeName == "gres") {
                gres = attributeValue;
            } else if (attributeName == "partition") {
                partition = attributeValue;
            } else if (attributeName == "account") {
                account = attributeValue;
            }
        }
        if (line.find("#SBATCH --job-name=") != string::npos) {
            jobName = line.substr(line.find('=') + 1);
        } else if (line.find("#SBATCH --cpus-per-task=") != string::npos) {
            cpusPerTask = stoi(line.substr(line.find('=') + 1));
        }
    }
    scriptFile.close();
    jobName = jobName.substr(0, jobName.find(" "));
    mem = mem.substr(0, mem.find(" "));
    gres = gres.substr(0, gres.find(" "));
    partition = partition.substr(0, partition.find(" "));
    account = account.substr(0, account.find(" "));
    char* cloned_file = clone_file(jobName, mem, gres, partition, account ,nTasks, cpusPerTask); 
}
int main(int argc, char *argv[]) {
    string init_start_time = get_high_priority_start_time();
    cout << init_start_time << endl;
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <arg1> <arg2> ..." << endl;
        return 1;
    }
    string filename = argv[1];
    regex pattern("\\.sh$");
    if (regex_search(filename, pattern)) {
        if (!fopen(argv[1], "r")) {
            cout << "File didn't exist.\n";
            throw runtime_error("File didn't exist.\n");
        }
        make_clone_script(filename, init_start_time);

    } else {
        cout << "Usage: " << argv[0] << " <arg1> <arg2> ..." << endl;
    }
    return 0;
}

