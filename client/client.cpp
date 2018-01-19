/********************************************************************************
    Client Program to interact with the processd

    @author     Harikrishnan P H
    @version    1.5
********************************************************************************/
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define DEFAULT_SOCKET_LOCATION "/home/hari/hari/ProcessMonitor/socket"
using namespace std;

enum CommandID
{
    BEG = 0,
    REGISTER,
    UNREGISTER,
    LIST,
    STATUS,
    EXIT,
    HELP,
    COMMANDS,
    END
};
int clientDesc = -1;

std::string commands[END - BEG -1];
std::string commandHelp[END - BEG -1];
std::string commandSyntax[END - BEG -1];

/*********************************************************************************
    Assign the commands and command help too an array

    @param  ->  NONE

    @return ->  NONE
*********************************************************************************/
void AssignArray()
{
    commands[REGISTER - 1].assign("REGISTER");
    commandHelp[REGISTER -1].assign("Registers an application with the process monitor thread\n"
                                        "Please set the appname and other values after adding the"
                                        "application");
    commandSyntax[REGISTER - 1].assign("register <App ID>");

    commands[UNREGISTER - 1].assign("UNREGISTER");
    commandHelp[UNREGISTER - 1].assign("Unregister an application with the process monitor daemon");
    commandSyntax[UNREGISTER - 1].assign("unregister <APP KEY>");

    /*commands[SET_PROPERTY - 1].assign("SET_PROPERTY");
    commandHelp[SET_PROPERTY - 1].assign("Sets the property of a registered application\n"
                                            "Possible Properties <name, location, argc, argv, priority, loglocation>");
    commandSyntax[SET_PROPERTY - 1].assign("set_property <App ID> <Property> <value>");*/

    /*commands[START_MONITOR - 1].assign("START_MONITOR");
    commandHelp[START_MONITOR - 1].assign("Starts the monitor of the application specified");
    commandSyntax[START_MONITOR - 1].assign("start_monitor <App ID>");*/

    commands[STATUS - 1].assign("STATUS");
    commandHelp[STATUS - 1].assign("Shows the status of the requested app");
    commandSyntax[STATUS - 1].assign("status <App ID>");

    commands[LIST - 1].assign("LIST");
    commandHelp[LIST - 1].assign("Lists all the apps registered with process monitor daemon\n"
                                            "list");
    commandSyntax[LIST - 1].assign("list");
    
    commands[HELP - 1].assign("HELP");
    commandHelp[HELP - 1].assign("Displays the help");
    commandSyntax[HELP - 1].assign("help");

    /*commands[SAVE_TO_FILE - 1].assign("SAVE_TO_FILE");
    commandHelp[SAVE_TO_FILE - 1].assign("Saves the changes made using client to the config file\n");*/

    commands[EXIT - 1].assign("EXIT");
    commandHelp[EXIT - 1].assign("Exits the client");

    /*commands[REMOVE - 1].assign("REMOVE");
    commandHelp[REMOVE - 1].assign("Removes data from the daemon");*/
    
    commands[COMMANDS - 1].assign("COMMANDS");
    commandHelp[COMMANDS - 1].assign("Displays all the commands");
}

/*********************************************************************************
    Checks whether the command entered by user is valid

    @param  ->  input   string entered by the user

    @return ->  -1  If the command is invalid
                >0  The index of the command if it is valid
*********************************************************************************/
int CheckValidityOfCommand(string input)
{
    string temp;
    int valid = 0, i, index;
    index = input.find(' ');
    if (std::string::npos == index)
    {
        index = input.length();
    }
    temp = input.substr(0, index);
    for (i = 0;i < END - BEG - 1;i++)
    {
        if (0 == commands[i].compare(temp))
        {
            valid = true;
            break;
        }
    }
    if (true == valid)
    {
        return i;
    }
    else
    {
        return -1;
    }
}

/*********************************************************************************
    Checks if the user entered valid args for the command

    @param  ->  input   String entered by the user
                command command id of the input from user

    @return ->  False   If the user entered invalid args
                True    If the user entered valid args
*********************************************************************************/
bool CheckValidArgs(int command, string input)
{
    //TODO check valid arg No and type
    return true;
}

/*********************************************************************************
    Sends a command to the Daemon

    @param  ->  input   String to be send to the Daemon

    @return ->  false   If sending to the daemon is success
                true    If sending to the daemon failed
*********************************************************************************/
bool SendCommandToDaemon(string input)
{
    int ret = -1;
    bool writeSuccess = true;
    ret = write(clientDesc, input.c_str(), input.length());
    if (ret < 0)
    {
        writeSuccess = false;
    }
    return writeSuccess;
}

/*********************************************************************************
    Get the response from the client

    @param  ->  response    The response from the client
                length      The length of response from client

    @return ->  false       If reading from client is a failure
                true        If reading from client is a success
*********************************************************************************/
bool GetDaemonResponse(string & response, int length = 100)
{
    char * responseArray = new char[length];
    int ret = -1;
    bool readSuccess = true;
    memset((void *)responseArray, 0, 100);
    ret = read(clientDesc, responseArray, length);
    if (ret <= 0)
    {
        readSuccess = false;
    }
    else
    {
        response.assign(responseArray);
    }
    delete responseArray;
    return readSuccess;
}

/*********************************************************************************
    Displays all the commands in the client

    @param  ->  NONE

    @return ->  NONE 
*********************************************************************************/
void DisplayCommands()
{
    int i = 0;
    for (i = 1;i < END - BEG ; i++)
    {
        cout << commands[i - 1] << "  " << endl;
    }
}

void ConvertToUpperCase(string & input);
/*********************************************************************************
    Displays the help message for the commands

    @param  ->   input   The input string entered by the user
                 If input has a space then display the help of commands after
                 the space
                 If input has no space display all the commands

    @return ->   NONE
*********************************************************************************/
//TODO check for whitespace separator and not space
void DisplayHelp(string input)
{
    int pos = -1;
    bool found = false;
    pos = input.find(" ");
    if (pos == std::string::npos)
    {
        int i = 0;
        for (i = 1;i < END - BEG -1; i++)
        {
            cout << commands[i - 1] << endl;
            cout << commandHelp[i - 1] << endl;
        }
    }
    else
    {
        string command = input.substr(pos + 1);
        for (int i = 1;i < END - BEG -1; i++)
        {
            ConvertToUpperCase(command);
            if (0 == commands[i - 1].compare(command))
            {
                cout << commandHelp[i - 1] << endl;
                found = true;
                break;
            }
        }
        if (!found)
        {
            cout << "No Such Command " << command << endl;
        }
    }
}

/*********************************************************************************
    Convert the command to upper case for sending

    @param  ->  The input entered by user

    @return ->  NONE
*********************************************************************************/
void ConvertToUpperCase(string & input)
{
    int i = 0; 
    int spacePos = input.find(' ');
    if (spacePos == std::string::npos)
    {
        spacePos = input.length();
    }
    for (i = 0;i < spacePos; i++)
    {
        if (input.at(i) >= 97 && input.at(i) <= 122)
        {
            input.at(i) -= 32;
        }
    }
}

/*********************************************************************************
    Returns the number of arguments which are space separated

    @param  ->  args    The string for checking

    @return ->  The number of space separated arguments
*********************************************************************************/
//TODO Check for whitespace instead of space
int CheckNumberOfArgs(string args)
{
    int count = 0;
    while (args.find(" ") != std::string::npos)
    {
        count++;
        args = args.substr(args.find(" ") + 1);
    }
    return count;
}

/*********************************************************************************
    Get a Mandatory field from the user and send it to the daemon

    @param  ->  field   the string which holds the field name

    @return ->  NONE
*********************************************************************************/
void SendMandatoryField(string field)
{
    string temp;
    do
    {
        temp.assign("");
        cout << field << ":";
        getline(std::cin, temp, '\n');
        if (temp.empty())
        {
            cout << field << " cannot be blank Please enter " << field << endl;
        }
    }while (temp.empty());
    SendCommandToDaemon(temp);
}

/*********************************************************************************
    Get an Optional field from the user and send it to the daemon

    @param  ->  field   the string which holds the field name

    @return ->  NONE
*********************************************************************************/
void SendOptionalField(string field)
{
    string temp;
    string choice;
    do
    {
        choice.assign("");
        cout << field << ":";
        getline(std::cin, temp, '\n');
        if (temp.empty())
        {
            cout << "Do you want to leave the field " << field <<" Empty (y/n):";
            getline(std::cin, choice, '\n');
        }
    } while (!choice.empty() || (0 == choice.compare("n")));
    if ((0 == choice.compare("y")) || choice.empty())
    {
        string temp("DEFAULT");
        SendCommandToDaemon(temp);
    }
}

/*********************************************************************************
    Sends all the Data needed for registering to the Daemon

    @param  ->  id  The appId of the app to be registered

    @return ->  NONE
*********************************************************************************/
void SendAppDataToDaemon(string id)
{
    string temp,field;
    cout << "Enter the App Data for " << id << endl;
    field.assign("AppName");
    SendMandatoryField(field);
    field.assign("AppLocation");
    SendMandatoryField(field);
    field.assign("LogLocation");
    SendOptionalField(field);
    field.assign("argv");
    SendOptionalField(field); 
    field.assign("argc");
    SendOptionalField(field);
    field.assign("Priority");
    SendOptionalField(field); 
}

/*********************************************************************************
    Send Register command to the daemon

    @param  ->  input   The data to be send to the daemon

    @return ->  NONE
*********************************************************************************/
void RegisterDevice(string input)
{
    int no = CheckNumberOfArgs(input);
    bool failed = false;
    if (no != 1)
    {
        cout << "Invalid Args for Register " << endl;
        cout << commandSyntax[REGISTER - 1];
    }
    else
    {
        string appId, response;
        appId.assign(input);
        if (!SendCommandToDaemon(input))
        {
            cout << "Failed to Send Command to Daemon" << endl;
        }
        else
        {
            if (GetDaemonResponse(response))
            {
                if (0 == response.compare("OK"))
                {
                    SendAppDataToDaemon(appId);
                }
                else if (0 == response.compare("ERROR"))
                {
                    cout << "The App Id you entered is already registered,\n"
                                 "Please enter another appId";
                    failed = true;
                }
            }
            if (!failed)
            {
                cout << "Here" <<endl;
                response.assign("");
                if (GetDaemonResponse(response))
                {
                    cout << response;
                }
            }
        }
    }
}

/*********************************************************************************
    Sends unregister command to the daemon

    @param  ->  input   The input from user

    @return ->  NONE
*********************************************************************************/
void UnRegisterDevice(string input)
{
    int no = CheckNumberOfArgs(input);
    bool failed = false;
    if (no != 1)
    {
        cout << "Invalid Args for UnRegister " << endl;
        cout << commandSyntax[UNREGISTER - 1];
    }
    else
    {
        if (!SendCommandToDaemon(input))
        {
            cout << "Failed to Send Command to Daemon" << endl;
        }
        else
        {
            string response;
            GetDaemonResponse(response);
            cout << response;
        }
    }
}

/*********************************************************************************
    Sends the List command to the daemon

    @param  ->  input   The input from User

    @return ->  NONE
*********************************************************************************/
void ListDevices(string input)
{
    int no = CheckNumberOfArgs(input);
    bool failed = false;
    if (no != 0)
    {
        cout << "Invalid Args for List " << endl;
        cout << commandSyntax[LIST - 1];
    }
    else
    {
        SendCommandToDaemon(input);
        int numberOfApps = 0;
        string response;
        GetDaemonResponse(response);
        numberOfApps = atoi(response.c_str());
        cout << response << endl;
        for (int i = 0;i< numberOfApps; i++)
        {
            GetDaemonResponse(response);
            cout << response << " , ";
            if (0 != i && i % 10 == 0)
            {
                cout << endl;
            }
        }
    }
}

/*********************************************************************************
    Return the why an app was restarted

    @param  ->  execCode    The error code for execv
                exitStatus  The exit status from waitpid
                message     The message containing reason for restart

    @return ->  NONE
*********************************************************************************/
void CheckCrashCode(unsigned int exitStatus, unsigned int execCode, string &message)
{
    if (0 == exitStatus && 0 == execCode)
    {
        message.assign("because the app was not started");
    }
    else if (0 == execCode)
    {
        //Check reason for exit
        if (WIFEXITED(exitStatus))
        {
            message.assign("because the execution was over\n The exit Code is ");
            string temp = to_string(WEXITSTATUS(exitStatus));
            message.append(temp);
        }
        else if (WIFSIGNALED(exitStatus))
        {
            message.assign("because it was terminated by a signal\n The signal number is ");
            string temp = to_string(WTERMSIG(exitStatus));
            message.append(temp);
        }
    }
    else if (0 == exitStatus)
    {
        //Check reason why app couldnt be started
        if (EACCES == execCode)
        {
            message.assign("because binary doesnt have correct permission");
        }
        else if (EFAULT == execCode)
        {
            message.assign("because arguments to the execv are wrong");
        }
        else if (ENOENT == execCode)
        {
            message.assign("because the binary was not present");
        }
        else if (ENOEXEC == execCode)
        {
            message.assign("beacuse the magic number is incorrect");
        }
    }
}

/*********************************************************************************
    Sends the status command to the daemon

    @param  ->  input   The input from the user

    @return ->  NONE
*********************************************************************************/
bool ShowStatus(string input)
{
    int no = CheckNumberOfArgs(input);
    bool failed = false;
    if (no != 1)
    {
        cout << "Invalid Args for Status " << endl;
        cout << commandSyntax[STATUS - 1];
    }
    else
    {
        string response;
        SendCommandToDaemon(input);
        int noOfCrashes = 0;
        int exitStatus = 0;
        int execCode = 0;
        string isRegistered;
        GetDaemonResponse(isRegistered);
        if (0 == isRegistered.compare("App Not Present"))
        {
            cout << isRegistered << endl;
            return false;
        }
        GetDaemonResponse(response);
        noOfCrashes = atoi(response.c_str());
        GetDaemonResponse(response);
        execCode = atoi(response.c_str());
        GetDaemonResponse(response);
        exitStatus = atoi(response.c_str());
        cout << "The app " << input.substr(input.find(" ") + 1) << " is " << isRegistered << endl;
        if (noOfCrashes >= 1)
        {
            if (1 == noOfCrashes)
            {
                cout << "It restarted " << noOfCrashes << " time" << endl;
            }
            else
            {
                cout << "It restarted " << noOfCrashes << " times" << endl;
            }
            string message;
            CheckCrashCode(exitStatus, execCode, message);
            cout << "Last restart was " << message << endl;
        }
    }
}

/*********************************************************************************
    Exits the client after sending EXIT command to the Daemon

    @param  ->  input   String to be sent to the daemon

    @return ->  NONE
*********************************************************************************/
bool ExitClient(string input)
{
    SendCommandToDaemon(input);
    exit(EXIT_SUCCESS);
}

/*********************************************************************************
    Decides what command user entered

    @param  ->  commandId   The index of the command
            ->  input       The input from the user

    @return ->  NONE
*********************************************************************************/
void ProcessCommand(int commandId, string input)
{
    switch(commandId + 1)
    {
        case REGISTER:
            RegisterDevice(input);
            break;
        case UNREGISTER:
            UnRegisterDevice(input);
            break;
        case LIST:
            ListDevices(input);
            break;
        case STATUS:
            ShowStatus(input);
            break;
        case HELP:
            DisplayHelp(input);
            break;
        case COMMANDS:
            DisplayCommands();
            break;
        case EXIT:
            ExitClient(input);
            break;
        default:
            cout << "InValid Command" << endl;
    }
}

void printHeader()
{
    cout << "-------------------------------------------------------" << endl;
    cout << "-------------------------------------------------------" << endl;
    cout << "               Process Monitor Client" << endl;
    cout << "               version 1.5" << endl;
    cout << endl;
    cout << "help       -> help on commands" <<  endl;
    cout << "commands   -> list of commands" <<  endl;
    cout << "-------------------------------------------------------" << endl;
    cout << "-------------------------------------------------------" << endl;
}

//TODO Give a header for the data passed through the sockets
/*********************************************************************************
    The Main of the client binary
*********************************************************************************/
int main()
{
    AssignArray();
    printHeader();
    char serverSocket[] = { DEFAULT_SOCKET_LOCATION };

    clientDesc = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un client_addr;
    if (-1 == clientDesc)
    {
        cout << "Cannot open connection to server " << endl;
        exit(EXIT_FAILURE);
    }
    client_addr.sun_family = AF_UNIX;
    strncpy(client_addr.sun_path, serverSocket, sizeof(client_addr.sun_path) - 1);
    if (-1 == connect(clientDesc, (struct sockaddr *)&client_addr, sizeof(client_addr)))
    {
        cout << "Cannot open connection to server " << endl;
        exit(EXIT_FAILURE);
    }
    string input;
    string response;
    int commandId;
    while(1)
    {
        cout << ">>";
        input.assign("");
        getline(std::cin, input,'\n');
        ConvertToUpperCase(input);
        commandId = CheckValidityOfCommand(input);
        if (input.empty())  continue;
        if (-1 == commandId)
        {
            //TODO Give suggestions to the user if command is Invalid
            cout << "Invalid Command" << endl;
            continue;
        }
        ProcessCommand(commandId, input);
        cout << "\n";
    }

    return 0;
}
