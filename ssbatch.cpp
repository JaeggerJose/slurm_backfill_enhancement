#include <iostream>
#include <stdexcept> // system command execute
#include <regex> // regular expression
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
    if (fgets(start_time, 256, pip) == nullptr)
        throw runtime_error("No job in the queue.\n");
    return start_time;
}

char* get_high_priority_start_time() {
    // get all user jobs in squeue and get the highest priority job's start time
    string command = "sprio --sort=-y -o \"%.15i\" | sed -n \"2p\"";
    vector<char> job_id(16);
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe)
        throw runtime_error("Failed to open command.");
    if (fgets(job_id.data(), 16, pipe) == NULL)
        throw runtime_error("No job in the queue.\n");
    string new_job_id = job_id.data();
    new_job_id.erase(0, new_job_id.find_first_not_of(' ')); // remove leading space
    return get_start_time(stoi(new_job_id));
}

char* clone_file(string jobName, string mem, string gres, string partition, string account, int nTasks, int cpusPerTask) {
    ofstream cloneFile;
    char* cloneFileName =  new char [256];
    sprintf(cloneFileName, "/tmp/%s_clone.sh", jobName.c_str());
    cloneFile.open(cloneFileName);
    cloneFile << "#!/bin/bash\n";
    cloneFile << "#SBATCH --job-name=" << jobName << "\n";
    cloneFile << "#SBATCH --mem=" << mem << "\n";
    cloneFile << "#SBATCH --gres=" << gres << "\n";
    cloneFile << "#SBATCH --partition=" << partition << "\n";
    cloneFile << "#SBATCH --account=" << account << "\n";
    cloneFile << "#SBATCH --ntasks=" << nTasks << "\n";
    cloneFile << "#SBATCH --cpus-per-task=" << cpusPerTask << "\n";
    cloneFile << "bash ";
    cloneFile.close();
    cout << cloneFileName << endl;
    return cloneFileName;
}

int make_clone_script(string filename) {
    ifstream scriptFile(filename);
    regex attributeRegex(R"(^#SBATCH\s+--(\w+)\s*=\s*(.*)$)");
    string line, jobName, mem, gres, partition, account;
    int nTasks = 0, cpusPerTask = 0;

    while (getline(scriptFile, line)) {
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

    char* cloned_file = new char [256];
    cloned_file = clone_file(jobName, mem, gres, partition, account ,nTasks, cpusPerTask);
    // sbatch the cloned file
    char* command = new char[256];
    sprintf(command, "sbatch %s", cloned_file);
    remove(cloned_file);
    FILE* pip = popen(command, "r");
    if(!pip)
       throw runtime_error("Failed to open command.");
    // output is "Submitted batch job 10246"
    char* job_id = new char [256];
    if (fgets(job_id, 256, pip) == nullptr)
        throw runtime_error("No job in the queue.\n");
    pclose(pip);
    cout << job_id;
    // useing regular rxpression to get the job id
    regex job_id_regex("\\d+");
    smatch match;
    string new_job_id = job_id;
    if(regex_search(new_job_id, match, job_id_regex))
        new_job_id = match.str();
    return stoi(new_job_id);
}

void scancel_job(int job_id) {
    char* command = new char[256];
    sprintf(command, "scancel %d", job_id);
    cout << command << endl;
    FILE* pip = popen(command, "r");
    if(!pip)
       throw runtime_error("Failed to open command.");
    pclose(pip);
    delete[] command;
}

char* get_user_name() {
    char* command = new char[256];
    sprintf(command, "whoami");
    FILE* pip = popen(command, "r");
    if(!pip)
       throw runtime_error("Failed to open command.");
    char* user_name = new char[256];
    if (fgets(user_name, 256, pip) == nullptr)
        throw runtime_error("No job in the queue.\n");
    pclose(pip);
    delete[] command;
    return user_name;
}

int main(int argc, char *argv[]) {
    string init_start_time = get_high_priority_start_time();
    int submit_job_id;
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <arg1> <arg2> ..." << endl;
        return 1;
    }
    string filename = argv[1];
    regex pattern("\\.sh$");
    if (regex_search(filename, pattern)) {
        if (!fopen(argv[1], "r"))
            throw runtime_error("File didn't exist.\n");
        submit_job_id = make_clone_script(filename);
        string aftre_start_time = get_high_priority_start_time();
        cout << init_start_time << "\t" << aftre_start_time << endl;
        bool start_time_is_same = aftre_start_time == init_start_time;

        scancel_job(submit_job_id);
        if(start_time_is_same) {
            // sbatch the original file
            char* submit_command = new char[256];
            sprintf(submit_command, "sbatch %s", filename.c_str());
            FILE* pipe = popen(submit_command, "r");
            if (!pipe)
                throw runtime_error("Failed to open command.");
            char buffer[256];
            while (!feof(pipe))
                if (fgets(buffer, 256, pipe) != NULL)
                    cout << buffer;
            pclose(pipe);
        } else {
            // make this job in the queue and check the queue every 1 seconds
            char* filepath = new char [256];
            char* username = new char [256];
            username = get_user_name();
            filepath = realpath(filename.c_str(), NULL);
            string tmp_filepath = string(filepath).substr(0, string(filepath).find_first_of('\n'));
            string tmp_username = string(username).substr(0,  string(username).find_first_of('\n'));
            delete[] filepath;
            delete[] username;
            ofstream queueFile;
            queueFile.open("/etc/enhancement_slurm/queue.txt", ios_base::app);
            queueFile << tmp_username << "\t" << tmp_filepath  << "\n";
            queueFile.close();
            }
    } else {
        cout << "Usage: " << argv[0] << " <arg1> <arg2> ..." << endl;
    }
    return 0;
}
