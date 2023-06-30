#include <iostream>
#include <fstream>
#include <string>

using namespace std;
class squeue {
    public:
    void print_job() {
            ifstream queueFile("/etc/enhancement_slurm/queue.txt");
        string line;
        string username;
        string filepath;
        cout << "USERNAME" << "\t" << "JOBPATH" << endl;
        while (getline(queueFile, line)) {
            size_t spacePos = line.find("\t");
             if (spacePos != string::npos) {
                // Extract the username and filepath based on the space position
                username = line.substr(0, spacePos);
                filepath = line.substr(spacePos + 1);
            }
            cout << username << "\t" << filepath << endl;
        }
    }
};

int main() {
    squeue s;
    s.print_job();
    return 0;
}