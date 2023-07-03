#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept> // system command execute
#include <regex> // regular expression
#include<unistd.h>

using namespace std;
class trace_queue {
    private:
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
        if (fgets(job_id.data(), 16, pipe) == NULL) {
            char* new_job_id = new char [256];
            sprintf(new_job_id, "START_DIRECTLY");
            return new_job_id;
        }
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
        cloneFile << "sleep 3456000";
        cloneFile.close();
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

    void sleep_function() {
        cout << "waiting for 1 min. for checking Slurm queue." << endl;
        sleep(30);
        cout << "30 seconds left." << endl;
        sleep(20);
        cout << "20 seconds left." << endl;
        sleep(10);
        return ;
    }

    void remove_job_from_queue(string username, string filepath) {
        ifstream queueFile("/etc/enhancement_slurm/queue.txt");
        ofstream tempFile("/etc/enhancement_slurm/temp.txt");
        string line;
        string file;
        while (getline(queueFile, line)) {
            size_t spacePos = line.find("\t");
            if (spacePos != string::npos) {
                // Extract the username and filepath based on the space position
                username = line.substr(0, spacePos);
                file = line.substr(spacePos + 1);
            }
            if (username != username && file != filepath) {
                tempFile << line << endl;
            }
        }
        queueFile.close();
        tempFile.close();
        remove("/etc/enhancement_slurm/queue.txt");
        rename("/etc/enhancement_slurm/temp.txt", "/etc/enhancement_slurm/queue.txt");
    }

    public:
    void trace() {
        ifstream queueFile("/etc/enhancement_slurm/queue.txt");
        string line;
        string username;
        string filepath;
        while (getline(queueFile, line)) {
           //step 1 get username and filepath
            string init_start_time = get_high_priority_start_time();
            size_t spacePos = line.find("\t");
            if (spacePos != string::npos) {
                // Extract the username and filepath based on the space position
                username = line.substr(0, spacePos);
                filepath = line.substr(spacePos + 1);
            }
            if (init_start_time == "START_DIRECTLY") {
                char* command = new char[256];
                sprintf(command, "sudo -u %s sbatch %s", username.c_str(), filepath.c_str());
                FILE* pip = popen(command, "r");
                if(!pip)
                throw runtime_error("Failed to open command.");
                delete[] command;
                // remove the job from the queuefile
                char* output = new char [1024];
                if (fgets(output, 1024, pip) == nullptr)
                    throw runtime_error("Invaild User\n");
                pclose(pip);
                remove_job_from_queue(username, filepath);

            }

            //step 2 clone the shell script and submit it to check the start time whether it is changed
            int new_job_id = make_clone_script(filepath);
            sleep_function();
            string aftre_start_time = get_high_priority_start_time();
            bool start_time_is_same = aftre_start_time == init_start_time;
            scancel_job(new_job_id);
            if(start_time_is_same) {
                //step 3 if the start time is not changed, then kill the job
                scancel_job(new_job_id);
                // sbatch the file with this username
                char* command = new char[256];
                sprintf(command, "sudo -u %s sbatch %s", username.c_str(), filepath.c_str());
                FILE* pip = popen(command, "r");
                if(!pip)
                throw runtime_error("Failed to open command.");
                delete[] command;
                // remove the job from the queuefile
                char* output = new char [1024];
                if (fgets(output, 1024, pip) == nullptr)
                    throw runtime_error("Invaild User\n");
                pclose(pip);
                remove_job_from_queue(username, filepath);
            } else {
                cout << filepath << "still cannot be sbatched!" << endl;
            }

        }
    }
};

int main() {
    trace_queue t;
    t.trace();
    return 0;
}
