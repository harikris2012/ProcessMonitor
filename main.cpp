#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include <string>
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <sstream>

#define LogLevel 0
#include "loglevel.h"
#include "appdata.h"
#define SI_SUPPORT_IOSTREAMS
#include "SimpleIni.h"
#define DEFAULT_CONFIG_FILE "/etc/ProcessMonitor/process.ini"
#define APP_NAME "a.out"

using namespace std;

CSimpleIniA file(true, true, true);
string validArgs[] = {"-p", ""};
string argsDef[] = {"For giving the path to the config file" , ""};
int noOfApps = 0;
AppsData * allData;

bool ParseProc(string searchApp, string & pid);

/*********************************************************************************
    The Process Monitor Thread
*********************************************************************************/
void ProcessMonitorThread(AppsData * data)
{
    //TODO parse the arguments, read pid from proc, 
    log_info("Started Monitor Thread for " << data->m_appName);
    string pid;
    char execFailed = 0;
    int fd[2] = {0};
    pipe2(fd, O_NONBLOCK);
    if (ParseProc(data->m_appName, pid))
    {
        data->m_pid = atoi(pid.c_str());
    }
    else
    {
        data->m_pid = -1;
    }
    while (data->m_isRegistered)
    {
        //Sends signal 0 to check if the process is running
        if ((data->m_pid > 0) && (0 == kill(data->m_pid, 0)))
        {
            sleep(data->m_priority * 5);
        }
        else
        {
            log_info("App " << data->m_appName << "  Crashed restarting the App");
            data->m_pid = fork();
            if (0 == data->m_pid)
            {
                close(fd[0]);
                execv(data->m_argsForExec[0], data->m_argsForExec);
                execFailed = errno;
                write(fd[1], &execFailed, sizeof(execFailed));
                exit(EXIT_FAILURE);
            }
            else
            {
                char failed = 0;
                close(fd[1]);
                waitpid(data->m_pid, &(data->m_exitStatus), 0);
                int ret = read(fd[0], &failed, sizeof(failed));
                data->m_execErrorCode = failed;
                if (ret < 0)
                {
                    log_error("Read failed");
                    continue;
                }
                if (0 != failed)
                {
                    log_error("Failed to Restart " << data->m_appName << "  stopping monitor exec error code is " << data->m_execErrorCode);
                    break;
                }
            }
        }
    }
}

/******************************************************************************
   Return the No of Apps in the Config file
    
    @param -> Takes the object having all the file data as argument

    @return -> Return the no of Apps Registered/Added in the config file
*******************************************************************************/

int GetNoOfSections(CSimpleIniA::TNamesDepend processess)
{
    CSimpleIni::TNamesDepend::const_iterator iSection = processess.begin();
    int noOfApps = 0;
    for( ; iSection != processess.end(); ++iSection, ++noOfApps);
    log_info("There are " << noOfApps << " Apps registered with Process Monitor Daemon");
    return noOfApps;
}

/*********************************************************************************
   Create a Data Structure for all the Process Data

   @return False->If failed to create the Data Structure
*********************************************************************************/
bool CreateDataStructure(CSimpleIniA::TNamesDepend  & processess)
{
    noOfApps = GetNoOfSections(processess);
    int i = 0;
    char * pTempArray;
    if (noOfApps)
    {
        allData = new AppsData[noOfApps];
        if (NULL == allData)
        {
            log_error("Failed to allocate memory");
            return false;
        }
    }
    CSimpleIni::TNamesDepend::const_iterator iSection = processess.begin();
    for ( ; iSection != processess.end(); ++iSection, ++i)
    {
        pTempArray = (char *)iSection->pItem;
        if (pTempArray != NULL)
        {
            allData[i].m_appID.assign(pTempArray);
        }
        else
        {
            log_error("Failed to read a Section Name");
            return false;
        }
        pTempArray = 0;
        pTempArray = (char *)file.GetValue(allData[i].m_appID.c_str(), "name", NULL);
        if (pTempArray != NULL)
        {
            allData[i].m_appName.assign(pTempArray);
        }
        else
        {
            log_error("Failed to read AppName of Section " << allData[i].m_appID);
            return false;
        }
        pTempArray = 0;
        pTempArray = (char *)file.GetValue(allData[i].m_appID.c_str(), "location", NULL);
        if (pTempArray != NULL)
        {
            allData[i].m_appLocation.assign(pTempArray);
        }
        pTempArray = 0;
        pTempArray = (char *)file.GetValue(allData[i].m_appID.c_str(), "argc", NULL);
        if (pTempArray != NULL)
        {
            allData[i].m_inputArgs.assign(pTempArray);
        }
        pTempArray = 0;
        pTempArray = (char *)file.GetValue(allData[i].m_appID.c_str(), "argv", NULL);
        if (pTempArray != NULL)
        {
            allData[i].m_noOfArgs = atoi(pTempArray);
        }
        pTempArray = 0;
        pTempArray = (char *)file.GetValue(allData[i].m_appID.c_str(), "priority", NULL);
        if (pTempArray != NULL)
        {
            allData[i].m_priority = atoi(pTempArray);
        }
        pTempArray = 0;
        pTempArray = (char *)file.GetValue(allData[i].m_appID.c_str(), "loglocation", NULL);
        if (pTempArray != NULL)
        {
            allData[i].m_logLocation.assign(pTempArray);
        }
    }
}

/*********************************************************************************
   Reads the data from config file using SimpleIni library

    @return     true:If Loading is Success
                false: If Loading is Failure 
*********************************************************************************/

bool ReadConfigFile(CSimpleIniA & file, const char * fileLocation)
{    
    log_info("Reading the config file");
    if (NULL == fileLocation)
    {
        SI_Error ret = file.LoadFile(DEFAULT_CONFIG_FILE);
        if (ret < 0)
        {
            log_error("Failed to Open config file... Exiting the Daemon");
            return false;
        }
    }
    else
    {
        SI_Error ret = file.LoadFile(fileLocation);
        if (ret < 0)
        {
            log_error("Failed to Open config file... Exiting the Daemon");
            return false;
        }
    }
    CSimpleIniA::TNamesDepend processess;
    file.GetAllSections(processess);
    //Creating the process data
    bool createSuccess = CreateDataStructure(processess);
}

/*********************************************************************************
   Checks if the command line argument given is valid
   
   @param   arg-> Command line argument given by User

   @return  The index of the argument entered if it is valid
            -1 if the Entered argument is invalid
**********************************************************************************/
int checkIfValidArg(char * arg)
{
    int i = 0;
    bool valid = false;
    while (!validArgs[i].empty())
    {
        if (0 == validArgs[i].compare(arg))
        {
            valid = true;
            return i;
        }
        i++;
    }
    if (false == valid)
    {
        return -1;
    }
}

/*********************************************************************************
*********************************************************************************/

void signal_handler(int flag)
{
    switch(flag)
    {
        case SIGINT:
        for (int i = 0; i < noOfApps; i++)
        {
            allData[i].StopThread();
        }
        delete []allData;
        log_info("Exiting Process Daemon");
        exit(0);
    }
}

/*********************************************************************************
*********************************************************************************/

void StartMonitor()
{
   for (int i = 0;i < noOfApps; i++)
    {
        allData[i].ProcessInputArgs();
        //allData[i].ShowClassValues();
        allData[i].StartThread();
    }
}

//TODO Allow only one instance of server in a machine
/*********************************************************************************
*********************************************************************************/

bool CheckIfServerIsRunning()
{
    // Try to Use a file and lock it on start of the program
    return false;
}

/*********************************************************************************
*********************************************************************************/
int main(int argc, char * argv[])
{

    CheckIfServerIsRunning();
    const char * fileLocation = NULL;
    
    signal(SIGINT, signal_handler);
    if (argc > 1)
    {
        int argType;
        for (int i = 1; i< argc; i++)
        {
            argType = checkIfValidArg(argv[i]);
            if (-1 == argType)
            {
                cout << "Invalid Args used" << endl;
                cout << "Supported Args are " << endl;
                for (int j = 0;validArgs[j].compare("") != 0; j++)
                {
                    cout << validArgs[j] << "->" << argsDef[j] << endl;
                }
                exit(EXIT_FAILURE);
            }
            else
            {
                if (0 == argType)
                {
                    if (i + 1 < argc)
                    {
                        fileLocation = argv[i+1];
                        i++;
                    }
                    else
                    {
                        log_error("Please enter the value for argument " << argv[i]);
                        return 1;
                    }
                }
            }
        }
    }

    log_info("Starting Monitor Daemon");
    ReadConfigFile(file, fileLocation);
    StartMonitor();
    
    while (1)
    {
        sleep(2);
    }
    //Deallocate the memory for class objects
    delete []allData;
    log_info("Exiting Monitor Daemon");
    return 0;
}
