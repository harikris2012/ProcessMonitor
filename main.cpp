#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
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
#define LOCK_FILE "/home/hari/ProcessMonitor_1.2/ProcessMonitor/lock"
#define MAX_CONNECTION 10
#define DEFAULT_SOCKET_PATH "/home/hari/ProcessMonitor_1.3/ProcessMonitor/socket"

using namespace std;

CSimpleIniA file(true, true, true);
string validArgs[] = {"-p", ""};
string argsDef[] = {"For giving the path to the config file" , ""};
int noOfApps = 0;
AppsData * allData;

bool ParseProc(string searchApp, string & pid);

/*********************************************************************************
*    The Process Monitor Thread
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
*   Return the No of Apps in the Config file
*    
*    @param     Takes the object having all the file data as argument
*
*    @return    Return the no of Apps Registered/Added in the config file
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
*   Create a Data Structure for all the Process Data
*
*   @return     False -> If failed to create the Data Structure
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
*   Reads the data from config file using SimpleIni library
*
*    @return     true:If Loading is Success
*                false: If Loading is Failure 
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
*   Checks if the command line argument given is valid
*   
*   @param   arg -> Command line argument given by User
*
*   @return  The index of the argument entered if it is valid
*            -1 if the Entered argument is invalid
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

void ReleaseResource()
{
    for (int i = 0; i < noOfApps; i++)
    {
        allData[i].StopThread();
    }
    delete []allData;
    unlink(DEFAULT_SOCKET_PATH);
}
/*********************************************************************************
*********************************************************************************/
void signal_handler(int flag)
{
    switch(flag)
    {
        case SIGINT:
        ReleaseResource();
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

enum commands
{
    BEG = 0,
    REGISTER = 1,
    UNREGISTER = 2,
    LIST = 3,
    STATUS = 4,
    EXIT = 5,
    END = 6
};

string processCommands[] = {"REGISTER", "UNREGISTER", "LIST", "STATUS", "EXIT"};

bool RegisterApp(string arg, int clientDesc)
{
    cout << "Registering an App" << endl;
    return true;
}

bool UnRegisterApp(string arg, int clientDesc)
{
    cout << "UnRegistering an App" << endl;
    return true;
}

bool ListRegisteredApps(string arg, int clientDesc)
{
    cout << "Listing Apps" << endl;
    return true;
}

bool ShowStatusOfApp(string arg, int clientDesc)
{
    cout << "App Status" << endl;
    return true;
}

/*********************************************************************************
*    Checks the client request and Decide which action to perform
*    
*    @param      Takes the command received from Client
*
*    @return     An Integer On success, Denoting the type of the request
*                -1 on Failure
*********************************************************************************/
int CheckCommand(string command)
{
    for (int i = 1; i < END - BEG - 1; i++)
    {
        if (0 == command.compare(processCommands[i - 1]))
        {
            return i;
        }
    }
    return -1;
}

/*********************************************************************************
*    Parses the string received from the client and extract the command and argument
*    
*    @param      buf     -> The buffer containing string send by the client
*                command -> The string to which command needs to be extracted
*                arg     -> The string to which command args are extracted
*    
*    @return     NONE
*********************************************************************************/
void GetCommandAndArgs(char * buf, string & command, string & arg)
{
    if (NULL != buf)
    {
        string temp;
        temp.assign(buf);
        int pos = temp.find(' ');
        command = temp.substr(0, pos);
        arg = temp.substr(pos + 1);
    }
}

/*********************************************************************************
    Process the client request and performs action corresponding to the request

    @param      buf -> The string send by the client

    @return     Returns     1   -> If user entered EXIT Command
                           -1   -> If user entered INVALID Command
*********************************************************************************/
int ProcessClientRequest(char buf[], int clientDesc)
{
    string arg, command;
    GetCommandAndArgs(buf, command, arg);
    switch(CheckCommand(command))
    {
        case REGISTER:
            RegisterApp(arg, clientDesc);
            break;
        case UNREGISTER:
            UnRegisterApp(arg, clientDesc);
            break;
        case LIST:
            ListRegisteredApps(arg, clientDesc);
            break;
        case STATUS:
            ShowStatusOfApp(arg, clientDesc);
            break;
        case EXIT:
            return 1;
            break;
        default://Invalid Request
            return -1;
    }
}

/*********************************************************************************
*   Handles the connection with the Client
*    
*   @param  The descripter of the socket connected to the client
*********************************************************************************/
void ClientConnectionHandler(int clientDesc)
{
    bool exitSend = false;
    char readBuf[100] = {0};
    int ret = 0;
    int readCheck;
    while (!exitSend)
    {
        if (-1 == clientDesc)
        {
            log_error("Failed to receive client messages");
            break;
        }
        readCheck = read(clientDesc, readBuf, 100);
        if (readCheck <= 0)
        //if (-1 == recv(clientDesc, readBuf, 100, 0))
        {
            log_error("Failed to read from Socket  " << errno);
            break;
        }
        else
        {
            ret = ProcessClientRequest(readBuf, clientDesc);
            if (1 == ret)
            {
                break;
            }
            else if (-1 == ret)
            {
                char buf[10] = "Invalid";
                if (write(clientDesc, buf, 7) == -1)
                {
                    log_error("Failed to get response");
                    exitSend = true;
                    continue;
                }
            }
        }
        printf("Buf %s\n", readBuf);
    }
    close(clientDesc);
}

/*********************************************************************************
*    Returns False if Server app is already running
*    
*    @return    true    -> If another instance of Daemon is running
*               false   -> If no other instance is running
*********************************************************************************/
bool CheckIfServerIsRunning()
{
    // Try to Use a file and lock it on start of the program
    int fd = open(LOCK_FILE, O_RDWR | O_CREAT, 0666);
    if (fd < 0)
    {
        log_error("Failed to Open Lock file, Exiting......");
        exit(EXIT_FAILURE);
    }
    int ret = flock(fd, LOCK_EX | LOCK_NB);
    if (-1 == ret)
    {
        return true;
    }
    return false;
}

//TODO Check the execv error code/waitpid error code and display the result
//TODO Need to be able to change the default socket location
/*********************************************************************************
*********************************************************************************/
int main(int argc, char * argv[])
{

    if (CheckIfServerIsRunning())
    {
        log_error("Another instance of the server is running,  Exiting.......");
        exit(EXIT_FAILURE);
    }
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
    
    //Check for Client request to create connection
    //Create Sockets for connecting to clients
    //First argument Socket Family IPV4/IPV6
    //Second argument is socket type SOCK_STREAM--TCP Type , SOCK_DGRAM -- UDP Type
    //Socket Protocol 0--default
    //Returns socket file Desc
    //Some of the types of Sockets are UNIX, INET, INETV6 ...etc,  UNIX is used for local IPC
    int socketDesc = socket(AF_UNIX, SOCK_STREAM, 0);
    int clientDesc = -1;
    struct sockaddr_un server_addr, client_addr;
    unsigned int clientSize = 0;
    if (-1 == socketDesc)
    {
        log_error("Failed to create socket end exiting....");
        exit(EXIT_FAILURE);
    }
    memset((void *)&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, DEFAULT_SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    //Bind the socket node to a socket file in the machine
    //Cannot bind
    if (-1 == bind(socketDesc, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_un)))
    {
        log_error("Failed to bind socket exiting.... ");
        exit(EXIT_FAILURE);
    }
    if (-1 == listen(socketDesc, MAX_CONNECTION))
    {
        log_error("Failed to listen, exiting");
        exit(EXIT_FAILURE);
    }
    clientSize = sizeof(struct sockaddr_un);
    std::thread clientThread;
    //while (clientDesc = accept(socketDesc, (struct sockaddr *)&client_addr, (socklen_t *)&clientSize))
    while (1)
    {
        //Accept connection from client and create a thread to process client requests
        clientDesc = accept(socketDesc, NULL, NULL);
        clientThread = std::thread(&ClientConnectionHandler, clientDesc);
        clientThread.detach();
    }
    //Deallocate the memory for class objects
    delete []allData;
    close(socketDesc);
    unlink(DEFAULT_SOCKET_PATH);
    log_info("Exiting Monitor Daemon");
    return 0;
}
