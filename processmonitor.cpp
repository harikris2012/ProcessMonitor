#include <iostream>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;
bool ParseProc(string searchApp, string & originalPid);
int main(int argc, char * argv[])
{
    static unsigned pid = -1;
    if (argc > 2 )
    {
        cout << "Invalid Usage" << endl;
    }
    string searchApp(argv[1]);
    string originalPid;
    if (!ParseProc(searchApp, originalPid))
    {
        return 0;
    }
    pid = atoi(originalPid.c_str());
    while (1)
    {
        if (0 == kill(pid, 0))
        {
            sleep(1);
            cout << "App is running" << endl;
        }
        else
        {
            cout << "App is not running" << endl;
            int status;
            pid = fork();
            char *  arg[] = { argv[1], NULL };
            if (0 == pid)
            {
                execv(arg[0], arg);
            }
            else
            {
                waitpid(pid, &status, 0);
                cout << "The  status is " << status << endl;
            }
        }
    }
    return 0;
}
