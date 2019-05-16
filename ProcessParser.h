#include <algorithm>
#include <iostream>
#include <math.h>
#include <thread>
#include <chrono>
#include <iterator>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include "constants.h"
#include <sys/stat.h> //included for implementing isPidExisting


using namespace std;

class ProcessParser{
    private:
        std::ifstream stream;
    public:
        static string getCmd(string pid);
        static vector<string> getPidList();
        static std::string getVmSize(string pid);
        static std::string getCpuPercent(string pid);
        static long int getSysUpTime();
        static std::string getProcUpTime(string pid);
        static string getProcUser(string pid);
        static vector<string> getSysCpuPercent(string coreNumber = "");
        static float getSysRamPercent();
        static string getSysKernelVersion();
        static int getTotalThreads();
        static int getTotalNumberOfProcesses();
        static int getNumberOfRunningProcesses();
        static string getOSName();
        static std::string PrintCpuStats(std::vector<std::string> values1, std::vector<std::string>values2);
        static bool isPidExisting(string pid);
        static int getNumberOfCores(); //add getNumberOfCores, which is missing
        static float getSysActiveCpuTime(vector<string> values); //add getSysActiveCpuTime, which is missing
        static float getSysIdleCpuTime(vector<string> values); //add getSysIdleCpuTime, which is missing
};

// TODO: Define all of the above functions below:

// implement getVMSize according to Lesson10.
std::string ProcessParser::getVmSize(string pid) {
    std::string line;
    std::string name = "VmData";
    std::ifstream stream;
    
    Util::getStream((Path::basePath() + pid + Path::statusPath()), stream); //modified the syntax according to the different implementation of getStream in the actual "util.h" file

    while(std::getline(stream, line)) {
        if(line.compare(0, name.size(), name) == 0) {
            std::istringstream buf(line);
            std::istream_iterator<string> beg(buf), end;
            std::vector<string> values(beg, end);
            return to_string(stof(values[1])/1024.f);
        };
    };
    return "";
};

// implement getCpuPercent according to Lesson11.
std::string ProcessParser::getCpuPercent(string pid) {
    float result;
    std::string line;
    std::ifstream stream;
    Util::getStream((Path::basePath() + pid + "/" + Path::statPath()), stream);

    std::getline(stream, line);
    std::istringstream buf(line);
    std::istream_iterator<string> beg(buf), end;
    std::vector<string> values(beg, end);

    float freq = sysconf(_SC_CLK_TCK);
   
    float utime = stof(ProcessParser::getProcUpTime(pid))*freq;  //the original expression in the video is wrong, getProcUpTime(pid) should be multiply by freq to ensure same unit with others.
    float stime = stof(values[14]);
    float cutime = stof(values[15]);
    float cstime = stof(values[16]);
    float starttime = stof(values[21]);

    float uptime = ProcessParser::getSysUpTime();
    float total_time = utime + stime + cutime + cstime;
    float seconds = uptime - (starttime/freq);

 //test code:
 /*   cout << "utime:   " << Util::convertToTime(utime/100)  << endl;
    cout << "stime:   " << Util::convertToTime(stime/100)  << endl;
    cout << "cutime:   " << Util::convertToTime(cutime/100)  << endl;
    cout << "cstime:   " << Util::convertToTime(cstime/100)  << endl;
    cout << "starttime:   " << Util::convertToTime(starttime/100)  << endl;
    cout << "uptime:   " << Util::convertToTime(uptime)  << endl;
    cout << "total_time:   " << Util::convertToTime(total_time/100)  << endl;
    cout << "seconds:   " << Util::convertToTime(seconds)  << endl; */
    
    result = 100.0*((total_time/freq)/seconds);
    return to_string(result);
};

// implement getProcUpTime according to Lesson12.
std::string ProcessParser::getProcUpTime(string pid) {
    std::string line;
    std::ifstream stream;
    Util::getStream((Path::basePath() + pid + "/" + Path::statPath()), stream);
    
    std::getline(stream, line);
    std::istringstream buf(line);
    std::istream_iterator<string> beg(buf), end;
    std::vector<string> values(beg, end);

    return to_string(float(stof(values[13])/sysconf(_SC_CLK_TCK)));
};

// implement getSysUpTime according to Lesson13.
long int ProcessParser::getSysUpTime() {
    std::string line;
    std::ifstream stream;
    Util::getStream((Path::basePath() + Path::upTimePath()), stream);

    std::getline(stream, line);
    std::istringstream buf(line);
    std::istream_iterator<string> beg(buf), end;
    std::vector<string> values(beg, end);

    return stoi(values[0]);
};

// implement getProcUser according to Lesson14.
string ProcessParser::getProcUser(string pid) {
    std::string line;
    std::string name = "Uid:";
    std::string result;
    
    std::ifstream s1, s2;
    Util::getStream((Path::basePath() + pid + Path::statusPath()), s1);

    while(std::getline(s1, line)) {
        if(line.compare(0, name.size(), name) == 0) {
            std::istringstream buf(line);
            std::istream_iterator<string> beg(buf), end;
            std::vector<string> values(beg, end);
            result = values[1];
            break;
        };
    };

    Util::getStream("/etc/passwd", s2);
    name = "x:" + result;
    while(std::getline(s2, line)) {
        if(line.find(name) != std::string::npos) {
            result = line.substr(0, line.find(":"));
            return result;
        };
    };

    return "";
};

// implement getPidList according to Lesson15.
vector<string> ProcessParser::getPidList() {
    DIR* dir;
    vector<std::string> container;

    if(!(dir = opendir("/proc"))) {
        throw std::runtime_error(std::strerror(errno));
    };

    while(dirent* dirp = readdir(dir)) {
        if(dirp->d_type != DT_DIR) 
            continue;

        if( all_of(dirp->d_name, dirp->d_name + std::strlen(dirp->d_name), [](char c){ return std::isdigit(c); })) {
            container.push_back(dirp->d_name);
        };
    };

    if(closedir(dir)) {
        throw std::runtime_error(std::strerror(errno));
    };

    return container;  
};

// implement getCmd according to Lesson16.
string ProcessParser::getCmd(string pid) {
    std::string line;
    std::ifstream stream;
    Util::getStream((Path::basePath() + pid + Path::cmdPath()), stream);
    std::getline(stream, line);
    return line;
};

// implement getNumberOfCores according to Lesson17.
int ProcessParser::getNumberOfCores() {
    std::string line;
    std::string name = "cpu cores";
    
    std::ifstream stream;
    Util::getStream((Path::basePath() + "cpuinfo"), stream);   
        
    while(std::getline(stream, line)) {
        if(line.compare(0, name.size(), name) == 0) {
            std::istringstream buf(line);
            std::istream_iterator<string> beg(buf), end;
            std::vector<string> values(beg, end);
            return stoi(values[3]);
        };
    }; 
    return 0;
};

// implement getSysCpuPercent according to Lesson18.
vector<string> ProcessParser::getSysCpuPercent(string coreNumber) {
    std::string line;
    std::string name = "cpu" + coreNumber;
    
    std::ifstream stream;
    Util::getStream((Path::basePath() + Path::statPath()), stream);   
        
    while(std::getline(stream, line)) {
        if(line.compare(0, name.size(), name) == 0) {
            std::istringstream buf(line);
            std::istream_iterator<string> beg(buf), end;
            std::vector<string> values(beg, end);
            return values;
        };
    }; 
    return {};
};

// implement getSysActiveCpuTime according to Lesson18.
float ProcessParser::getSysActiveCpuTime(vector<string> values) {
    return (stof(values[S_USER]) +
            stof(values[S_NICE]) +
            stof(values[S_SYSTEM]) +
            stof(values[S_IRQ]) +
            stof(values[S_SOFTIRQ]) +
            stof(values[S_STEAL]) +
            stof(values[S_GUEST]) +
            stof(values[S_GUEST_NICE]));
};

// implement getSysIdleCpuTime according to Lesson18.
float ProcessParser::getSysIdleCpuTime(vector<string> values) {
    return (stof(values[S_IDLE]) +
            stof(values[S_IOWAIT]));
};

// implement PrintCpuStats according to Lesson19.
std::string ProcessParser::PrintCpuStats(std::vector<std::string> values1, std::vector<std::string>values2) {
    float activeTime = getSysActiveCpuTime(values2) - getSysActiveCpuTime(values1);
    float idleTime = getSysIdleCpuTime(values2) - getSysIdleCpuTime(values1);
    float totalTime = activeTime + idleTime;
    return to_string(100.*activeTime/totalTime);
};

// implement getSysRamPercent according to Lesson20. using for-loop instead of repeating similar code 3 times
float ProcessParser::getSysRamPercent() {
    std::string line;
    std::vector<std::string> names = {"MemAvailable:", "MemFree:", "Buffers:"};
    std::vector<float> result = {0., 0., 0.};
    std::ifstream stream;
    Util::getStream((Path::basePath() + Path::memInfoPath()), stream);


    while(std::getline(stream, line)) {
        int counter = 0;
        for (std::string name : names) {
            if(line.compare(0, name.size(), name) == 0) {
                std::istringstream buf(line);
                std::istream_iterator<string> beg(buf), end;
                std::vector<string> values(beg, end);
                result[counter] = stof(values[1]);
            };
            counter++;
        };
    }; 
    return float(100.*(1-(result[1]/(result[0] - result[2]))));
};

// implement getSysKernelVersion according to Lesson20.
string ProcessParser::getSysKernelVersion() {
    std::string name = "Linux version ";
    std::string line;
    std::ifstream stream;
    Util::getStream((Path::basePath() + Path::versionPath()), stream);

    std::getline(stream, line);
    if(line.compare(0, name.size(), name) == 0) {
        std::istringstream buf(line);
        std::istream_iterator<string> beg(buf), end;
        std::vector<string> values(beg, end);
        return values[2];
    };
    return "";
};

// implement getOSName according to Lesson20.
string ProcessParser::getOSName() {
    std::string name = "PRETTY_NAME=";
    std::string line;
    std::string result;
    std::ifstream stream;
    Util::getStream("/etc/os-release", stream);

    while(std::getline(stream, line)) {
        if(line.compare(0, name.size(), name) == 0) {
            std::size_t found = line.find("=");
            found++;
            result = line.substr(found);
            result.erase(remove(result.begin(), result.end(), '"'), result.end());
            return result;
        };
    };
    return "";
};

// implement getTotalThreads according to Lesson20.
int ProcessParser::getTotalThreads() {
    std::string line;
    std::string name = "Threads:";
    std:vector<std::string> pidList = getPidList();
    
    int Threads = 0;

    for (std::string pid : pidList) {
        std::ifstream stream;
        Util::getStream((Path::basePath() + pid + Path::statusPath()), stream);
        while(std::getline(stream, line)) {
            if(line.compare(0, name.size(), name) == 0) {
                std::istringstream buf(line);
                std::istream_iterator<string> beg(buf), end;
                std::vector<string> values(beg, end);
                Threads += stoi(values[1]);
                //cout << Threads << endl; //test code
                break;
            };
        };
    };
    return Threads;
};

// implement getTotalNumberOfProcesses according to Lesson20.
int ProcessParser::getTotalNumberOfProcesses() {
    std::string line;
    std::string name = "processes";
    std::ifstream stream;

    Util::getStream((Path::basePath() + Path::statPath()), stream);
    while(std::getline(stream, line)) {
        if(line.compare(0, name.size(), name) == 0) {
            std::istringstream buf(line);
            std::istream_iterator<string> beg(buf), end;
            std::vector<string> values(beg, end);
            return stoi(values[1]);
        };
    };
    return 0;
};

// implement getNumberOfRunningProcesses according to Lesson20.
int ProcessParser::getNumberOfRunningProcesses() {
    std::string line;
    std::string name = "procs_running";
    std::ifstream stream;

    Util::getStream((Path::basePath() + Path::statPath()), stream);
    while(std::getline(stream, line)) {
        if(line.compare(0, name.size(), name) == 0) {
            std::istringstream buf(line);
            std::istream_iterator<string> beg(buf), end;
            std::vector<string> values(beg, end);
            return stoi(values[1]);
        };
    };
    return 0;
};

// implement isPidExisting, modified from https://stackoverflow.com/questions/18100097/portable-way-to-check-if-directory-exists-windows-linux-c 
bool ProcessParser::isPidExisting(string pid) {
    struct stat info;
    std:string path = Path::basePath() + pid + "/";
    return (stat(path.c_str(), &info) == 0);
};