/********************************************************************
    Process Monitor Daemon

    @author     Harikrishnan P H
    @version    1.5
********************************************************************/
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
#include <vector>
#include <condition_variable>

#define LogLevel 0
#include "loglevel.h"
#include "appdata.h"
#define SI_SUPPORT_IOSTREAMS
#include "SimpleIni.h"
#define DEFAULT_CONFIG_FILE "/etc/ProcessMonitor/process.ini"
#define LOCK_FILE "/home/hari/hari/ProcessMonitor/lock"
#define MAX_CONNECTION 10
#define DEFAULT_SOCKET_PATH "/home/hari/hari/ProcessMonitor/socket"

using namespace std;

CSimpleIniA file(true, true, true);
string validArgs[] = {"-p", ""};
string argsDef[] = {"For giving the path to the config file" , ""};
int noOfApps = 0;
std::vector<AppsData> allData;
const char * fileLocation = NULL;

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
//    while (data->m_isRunning)
//    {
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
                data->m_noOfCrashes++;
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
                    //data->m_execErrorCode = -1;
                    //data->m_exitStatus = -1;
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
                        log_error("Failed to Restart " << data->m_appName << 
                                     "  stopping monitor exec error code is " << data->m_execErrorCode
                                     << " \nexit Status is " << data->m_exitStatus);
                        data->m_isRegistered = false;
                        break;
                    }
                }
            }
        }
//        data->m_threadMutex.lock();
//        data->m_condition.wait(data->m_threadMutex);
//        data->m_threadMutex.unlock();
//    }
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
    /*if (noOfApps)
    {
        allData = new AppsData[noOfApps];
        if (NULL == allData)
        {
            log_error("Failed to allocate memory");
            return false;
        }
    }*/
    
    CSimpleIni::TNamesDepend::const_iterator iSection = processess.begin();
    for ( ; iSection != processess.end(); ++iSection, ++i)
    {
        AppsData newData;
        pTempArray = (char *)iSection->pItem;
        if (pTempArray != NULL)
        {
            newData.m_appID.assign(pTempArray);
        }
        else
        {
            log_error("Failed to read a Section Name");
            return false;
        }
        pTempArray = 0;
        pTempArray = (char *)file.GetValue(newData.m_appID.c_str(), "name", NULL);
        if (pTempArray != NULL)
        {
            newData.m_appName.assign(pTempArray);
        }
        else
        {
            log_error("Failed to read AppName of Section " << newData.m_appID);
            return false;
        }
        pTempArray = 0;
        pTempArray = (char *)file.GetValue(newData.m_appID.c_str(), "location", NULL);
        if (pTempArray != NULL)
        {
            newData.m_appLocation.assign(pTempArray);
        }
        pTempArray = 0;
        pTempArray = (char *)file.GetValue(newData.m_appID.c_str(), "argc", NULL);
        if (pTempArray != NULL)
        {
            newData.m_inputArgs.assign(pTempArray);
        }
        pTempArray = 0;
        pTempArray = (char *)file.GetValue(newData.m_appID.c_str(), "argv", NULL);
        if (pTempArray != NULL)
        {
            newData.m_noOfArgs = atoi(pTempArray);
        }
        pTempArray = 0;
        pTempArray = (char *)file.GetValue(newData.m_appID.c_str(), "priority", NULL);
        if (pTempArray != NULL)
        {
            newData.m_priority = atoi(pTempArray);
        }
        pTempArray = 0;
        pTempArray = (char *)file.GetValue(newData.m_appID.c_str(), "loglocation", NULL);
        if (pTempArray != NULL)
        {
            newData.m_logLocation.assign(pTempArray);
        }
        allData.push_back(newData);
    }
    return true;
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

/*********************************************************************************
    Stops all the threads and deletes the socket file

    @param  ->  NONE

    @return ->  NONE
*********************************************************************************/
void ReleaseResource()
{
    for (int i = 0; i < allData.size(); i++)
    {
        allData.at(i).StopThread();
    }
    unlink(DEFAULT_SOCKET_PATH);
}

/*********************************************************************************
    Signal Handler for handling SIGINT, SIGHUP

    @param  ->  flag    signal flag

    @return ->  NONE
*********************************************************************************/
void signal_handler(int flag)
{
    switch(flag)
    {
        case SIGINT:
        ReleaseResource();
        log_info("Exiting Process Daemon");
        exit(0);
        break;
        case SIGHUP:
        ReleaseResource();
        log_info("Exiting Process Daemon");
        exit(0);
    }
}

/*********************************************************************************
    Starts the Monitor of all the apps in the config file

    @param  ->  NONE

    @return ->  NONE
*********************************************************************************/
void StartMonitor()
{
    for (int i = 0;i < allData.size(); i++)
    {
        allData.at(i).ProcessInputArgs();
        allData.at(i).ShowClassValues();
        allData.at(i).StartThread();
    }
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

/*********************************************************************************
*    Parses the command line arguments for the Daemon
*
*    @param  ->  argc    argc of the main
*            ->  argv    argv of the main
*********************************************************************************/
void ParseCommandLineArgs(int argc, char * argv[])
{
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
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }
}


/*******************************SERVER CODE BEGIN********************************/
enum commands
{
    BEG = 0,
    REGISTER,
    SET_PROPERTY,
    START_MONITOR,
    UNREGISTER,
    LIST,
    STATUS,
    SAVE_TO_FILE,
    REMOVE,
    EXIT,
    END
};

string processCommands[] = {"REGISTER", "SET_PROPERTY", "START_MONITOR",
                                 "UNREGISTER", "LIST", "STATUS", "SAVE_TO_FILE", "REMOVE", "EXIT"};

//TODO prepend the send data with a header
/*********************************************************************************
*    Writes the Response for the client
*
*    @param  ->  clientDesc  Descriptor for communicating with the client
*            ->  message     Message to be passed to the client
*
*    @return ->  NONE
*********************************************************************************/
void SendResponse(int clientDesc, string message)
{
    int ret = -1;
    ret = write(clientDesc, message.c_str(), message.length());
}

//TODO Parse the header and understand the send item length
/*********************************************************************************
*    Reads the data send by the client
*
*    @param  ->  clientDesc  Descriptor for communicating with the client
*            ->  request     Variable for storing the read value
*
*    @return ->  NONE
*********************************************************************************/
void ReadRequest(int clientDesc, string & request)
{
    int ret = -1;
    char buf[100] = {0};
    ret = read(clientDesc, buf, 100);
    request.assign(buf);
}

/*********************************************************************************
*    Checks whether and AppId is already registered with the Daemon
*
*    @param  ->  id  Identifier for checking
*
*    @return ->  -1  If No match found
*                >0  If match is found(Position of the found AppId)
*********************************************************************************/
int CheckAppIdIsPresent(string id)
{
    bool isPresent = false;
    int i;
    for (i = 0; i < allData.size(); i++)
    {
        cout << "  " << allData.at(i).m_appID << "  " << id << endl;
        if (0 == allData.at(i).m_appID.compare(id))
        {
            isPresent = true;
            break;
        }
    }
    if (isPresent)
    {
        return i;
    }
    else
    {
        return -1;
    }
}

/*********************************************************************************
*    Read the data for the new Application from the client
*
*    @param  ->  clientDesc  File descriptor for interacting with the client
*            ->  newDat      Variable for storing the data read from client
*
*    @return ->  NONE
*********************************************************************************/
void GetNewAppData(int clientDesc, AppsData & newDat)
{
    string temp;
    //Read AppName Value
    ReadRequest(clientDesc, temp);
    newDat.m_appName.assign(temp);
    //Read AppLocation Value
    ReadRequest(clientDesc, temp);
    newDat.m_appLocation.assign(temp);
    //Read LogLocation Value
    ReadRequest(clientDesc, temp);
    if (0 != temp.compare("DEFAULT"))
    {
        newDat.m_logLocation.assign(temp);
    }
    //Read argv value
    ReadRequest(clientDesc, temp);
    if (0 != temp.compare("DEFAULT"))
    {
        newDat.m_noOfArgs = atoi(temp.c_str());
    }
    //Read argc value
    ReadRequest(clientDesc, temp);
    if (0 != temp.compare("DEFAULT"))
    {
        newDat.m_inputArgs.assign(temp);
    }
    //Read priority value
    ReadRequest(clientDesc, temp);
    if (0 != temp.compare("DEFAULT"))
    {
        newDat.m_priority = atoi(temp.c_str());
    }
}

/*********************************************************************************
    Adds a Registered Data to the File

    @param  ->  newData     Data to be added to the file

    @return ->  NONE
*********************************************************************************/
void AddDataToFile(AppsData &newData)
{
    file.SetValue(newData.m_appID.c_str(), "name", newData.m_appName.c_str());
    file.SetValue(newData.m_appID.c_str(), "location", newData.m_appLocation.c_str());
    file.SetValue(newData.m_appID.c_str(), "loglocation", newData.m_logLocation.c_str());
    if (0 != newData.m_inputArgs.compare("NOARGS"))
    {
        file.SetValue(newData.m_appID.c_str(), "argc", newData.m_inputArgs.c_str());
    }
    if (0 != newData.m_noOfArgs)
    {
        string temp = to_string(newData.m_noOfArgs);
        file.SetValue(newData.m_appID.c_str(), "argv", temp.c_str());
    }
    if (10 != newData.m_priority)
    {
        string temp = to_string(newData.m_priority);
        file.SetValue(newData.m_appID.c_str(), "priority", temp.c_str());
    }
    if (NULL == fileLocation)
    {
        file.SaveFile(DEFAULT_CONFIG_FILE);
    }
    else
    {
        file.SaveFile(fileLocation);
    }
}

/*********************************************************************************
*    Registers an Application with the Monitor Daemon
*    
*    @param       arg        ->   Command received by the Server
*                 clientDesc ->   File Descriptor for interacting with client
*
*    @return      bool       ->   True,  If the operation is Success
*                                 False, If the operation is Failure
*********************************************************************************/
bool RegisterApp(string arg, int clientDesc)
{
    cout << "Registering an App " << arg << endl;
    string message, appId;
    appId.assign(arg);
    int no;
    no = CheckAppIdIsPresent(appId);
    if (0 <= no)
    {
        //ERROR should mean app already registered
        message.assign("ERROR");
        SendResponse(clientDesc, message);
        return false;
    }
    else
    {
        message.assign("OK");
        SendResponse(clientDesc, message);
        AppsData newData;
        GetNewAppData(clientDesc, newData);
        newData.m_appID.assign(appId);
        allData.push_back(newData);
        allData.back().ProcessInputArgs();
        //allData.back().ShowClassValues();
        allData.back().StartThread();
        AddDataToFile(allData.back());
        message.assign("OK");
        SendResponse(clientDesc, message);
        noOfApps++;
    }
    return true;
}

/*********************************************************************************
*    Unregisters an Application with the Daemon, Currently only pauses the monitoring
*    To Delete the data from config file call save to file  
* 
*    @param       arg        ->   Command received by the Server
*                 clientDesc ->   File Descriptor for interacting with client
*
*    @return      bool       ->   True,  If the operation is Success
*                                 False, If the operation is Failure
*********************************************************************************/
bool UnRegisterApp(string arg, int clientDesc)
{
    string message, appId;
    cout << "UnRegistering an App" << endl;
    appId.assign(arg);
    int no;
    no = CheckAppIdIsPresent(appId);
    if (-1 == no)
    {
        message.assign("App Not Present");
        SendResponse(clientDesc, message);
    }
    else
    {
        if (allData.at(no).IsStopped())
        {
            message.assign("Already Stopped");
            SendResponse(clientDesc, message);
        }
        else
        {
            allData.at(no).StopThread();
            //Delete the Section from file
            file.Delete(allData.at(no).m_appID.c_str(), NULL);
            if (NULL == fileLocation)
            {
                file.SaveFile(DEFAULT_CONFIG_FILE);
            }
            else
            {
                file.SaveFile(fileLocation);
            }
            message.assign("OK");
            SendResponse(clientDesc, message);
        }
    }
    return true;
}

/*********************************************************************************
*    Used for changing any property of an App after it is Registered
*    
*    @param       arg        ->   Command received by the Server
*                 clientDesc ->   File Descriptor for interacting with client
*
*    @return      bool       ->   True,  If the operation is Success
*                                 False, If the operation is Failure
*********************************************************************************/
bool ListRegisteredApps(string arg, int clientDesc)
{
    string message;
    cout << "Listing Apps"<< endl;
    message = to_string(allData.size());
    SendResponse(clientDesc, message);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    for (int i = 0;i < allData.size();i++)
    {
        message.assign(allData.at(i).m_appID);
        SendResponse(clientDesc, message);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return true;
}

/*********************************************************************************
*    Displays the Status of the requested application, with information like
*    Whether application is currently monitored, No of Crashes, Total Run Time
*    Last Crash Cause...etc
*    
*    @param       arg        ->   Command received by the Server
*                 clientDesc ->   File Descriptor for interacting with client
*
*    @return      bool       ->   True,  If the operation is Success
*                                 False, If the operation is Failure
*********************************************************************************/
bool ShowStatusOfApp(string arg, int clientDesc)
{
    string message, appId;
    cout << "App Status" << endl;
    appId.assign(arg);
    int no = CheckAppIdIsPresent(appId);
    if (-1 == no)
    {
        message.assign("App Not Present");
        SendResponse(clientDesc, message);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    else
    {
        if (allData.at(no).m_isRegistered)
        {
            message.assign("Monitored");
            SendResponse(clientDesc, message);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        else
        {
            message.assign("Not Monitored");
            SendResponse(clientDesc, message);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        message = to_string(allData.at(no).m_noOfCrashes);
        SendResponse(clientDesc, message);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        message = to_string(allData.at(no).m_execErrorCode);
        SendResponse(clientDesc, message);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        message = to_string(allData.at(no).m_exitStatus);
        SendResponse(clientDesc, message);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return true;
}

/*********************************************************************************
*    Used for changing any property of an App after it is Registered
*    
*    @param       arg        ->   Command received by the Server
*                 clientDesc ->   File Descriptor for interacting with client
*
*    @return      bool       ->   True,  If the operation is Success
*                                 False, If the operation is Failure
*********************************************************************************/
//TODO For future Implementation
bool SetProperty(string arg, int clientDesc)
{
    string message;
    cout << "Set property" << endl;
    return true;
}

/*********************************************************************************
*    Starts the Monitor for any Paused Application
*    
*    @param       arg        ->   Command received by the Server
*                 clientDesc ->   File Descriptor for interacting with client
*
*    @return      bool       ->   True,  If the operation is Success
*                                 False, If the operation is Failure
*********************************************************************************/
//TODO For future Implementation
bool StartMonitorThread(string args, int clientDesc)
{
    string message;
    cout << "Start the monitor of app" <<  endl;
    return true;
}

/*********************************************************************************
*    Saves the current Changes to the config file
*
*    @param       arg        ->   Command received by the Server
*                 clientDesc ->   File Descriptor for interacting with client
*
*    @return      bool       ->   True,  If the operation is Success
*                                 False, If the operation is Failure
*********************************************************************************/
//TODO For future Implementation
bool SaveToFile(string args, int clientDesc)
{
    string message;
    cout << "Save to file" << endl;
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
    for (int i = 1; i < END - BEG ; i++)
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
        if (pos != std::string::npos)
        {
            command = temp.substr(0, pos);
            arg = temp.substr(pos + 1);
        }
        else
        {
            command.assign(temp);
            arg.assign("");
        }
    }
}

/*********************************************************************************
*    Process the client request and performs action corresponding to the request
*
*    @param      buf -> The string send by the client
*
*    @return     Returns     1   -> If user entered EXIT Command
*                           -1   -> If user entered INVALID Command
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
        for (int i = 0; i < 100; i++)
        {
            readBuf[i] = 0;
        }
        readCheck = read(clientDesc, readBuf, 100);
        if (readCheck <= 0)
        {
            log_error("Failed to read from Socket  " << errno);
            break;
        }
        else
        {
            ret = ProcessClientRequest(readBuf, clientDesc);
            if (1 == ret)
            {
                log_info("User entered exit stop request handler");
                break;
            }
            else if (-1 == ret)
            {
                char buf[10] = "Invalid";
                int send = write(clientDesc, buf, 7);
                if (send <= 0)
                {
                    log_error("Failed to get response");
                    exitSend = true;
                    continue;
                }
            }
        }
    }
    close(clientDesc);
}

/*********************************************************************************
*    Main function for the server part of the Process Monitor daemon
*    
*    Creates a UNIX socket and wait for connction request from any client
*********************************************************************************/
void ServerMain()
{
    //Check for Client request to create connection
    //Create Sockets for connecting to clients
    //First argument Socket Family IPV4/IPV6
    //Second argument is socket type SOCK_STREAM--TCP Type , SOCK_DGRAM -- UDP Type
    //Socket Protocol 0--default
    //Returns socket file Desc
    //Some of the types of Sockets are UNIX, INET, INETV6 ...etc,  UNIX is used for local IPC
    unlink(DEFAULT_SOCKET_PATH);
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
    close(socketDesc);
 }

/*******************************SERVER CODE END**********************************/

//TODO Check the execv error code/waitpid error code and display the result
//TODO Need to be able to change the default socket Allocation


/*********************************************************************************
*                           Main of the Process Daemon
*    Reads a config file and stores the data into a Data Structure
*    Starts monitoring for the data read from the config file
*    Start a Server socket to listen for any requests
**********************************************************************************/
int main(int argc, char * argv[])
{

    if (CheckIfServerIsRunning())
    {
        log_error("Another instance of the server is running,  Exiting.......");
        exit(EXIT_FAILURE);
    }
    
    signal(SIGINT, signal_handler);
    ParseCommandLineArgs(argc, argv);
    
    log_info("Starting Monitor Daemon");
    if (!ReadConfigFile(file, fileLocation))
    {
        exit(EXIT_FAILURE);
    }
    StartMonitor();
    
    ServerMain();
    //Deallocate the memory for class objects
    unlink(DEFAULT_SOCKET_PATH);
    log_info("Exiting Monitor Daemon");
    return 0;
}
