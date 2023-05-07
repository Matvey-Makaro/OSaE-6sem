#include <csignal>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <ctime>
#include <chrono>
#include <map>
#include <list>

bool run = true;
std::ofstream fout;
std::list<int> signals_to_log;

void signal_handler(int signum);

void daemonize()
{
    pid_t pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
        exit(EXIT_SUCCESS);

    if (setsid() < 0)
        exit(EXIT_FAILURE);

    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
    {
        std::ofstream pid_fout(".pid");
        pid_fout << pid << "\n";
        pid_fout.close();
        exit(EXIT_SUCCESS);
    }

    umask(0);

    int x;
    for (x = static_cast<int>(sysconf(_SC_OPEN_MAX)); x >= 0; x--)
    {
        close(x);
    }
}

std::string time_point_to_string(const std::chrono::system_clock::time_point& timePoint)
{
    std::time_t tpTimer = std::chrono::system_clock::to_time_t(timePoint);
    std::tm tpTm = *std::localtime(&tpTimer);
    char buffer[100];
    strftime(buffer, sizeof(buffer), "%c", &tpTm);
    return buffer;
}

std::string get_curr_timestamp()
{
    return time_point_to_string(std::chrono::system_clock::now());
}

void log(const std::string& msg)
{
    if (!fout.is_open()) { fout.open("logs.txt"); }
    fout << get_curr_timestamp() << ": " << msg << "\n";
    fout.flush();
}

std::list<int> read_signals_to_log()
{
    std::ifstream fin("config.txt");
    std::list<int> signals;
    int curr_sig;
    fin >> curr_sig;
    while (fin)
    {
        signals.push_back(curr_sig);
        fin >> curr_sig;
    }

    fin.close();

    return signals;
}

void reconfigure()
{
    for (int sig : signals_to_log)
        signal(sig, SIG_DFL);

    signals_to_log = read_signals_to_log();
    for (int sig : signals_to_log)
        signal(sig, signal_handler);

    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
}

void signal_handler(int signum)
{
    log(std::string("caught signal ") + strsignal(signum) +
        std::string(". Signum: ") + std::to_string(signum));

    if (signum == SIGTERM)
    {
        run = false;
    }
    else if (signum == SIGHUP)
    {
        reconfigure();
    }
}

void stop_daemon()
{
    std::ifstream fin(".pid");
    int pid;
    fin >> pid;
    fin.close();
    kill(pid, SIGTERM);
}

int main(int argc, char* argv[])
{
    if (argc > 1)
    {
        if (std::string(argv[1]) != "stop")
        {
            std::cerr << "Invalid command. Use stop to stop the daemon";
            exit(EXIT_FAILURE);
        }
        else
        {
            stop_daemon();
            exit(EXIT_FAILURE);
        }
    }

    daemonize();
    log("daemon started");

    reconfigure();

    while (run)
    {
        usleep(1000);
    }

    log("daemon terminated");
    fout.close();
    remove(".pid");

    return 0;
}