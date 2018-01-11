/********************************************************************************
    Client Program to interact with the processd

    @author     Harikrishnan P H
    @version    1.3
********************************************************************************/
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <unistd.h>

#define DEFAULT_SOCKET_LOCATION "/home/hari/ProcessMonitor_1.3/ProcessMonitor/socket"
using namespace std;

enum CommandID
{
    BEG = 0,
    REGISTER,
    SET_PROPERTY,
    START_MONITOR,
    UNREGISTER,
    LIST,
    STATUS,
    EXIT,
    HELP,
    SAVE_TO_FILE,
    END
};
int clientDesc = -1;

std::string commands[END - BEG -1];
std::string commandHelp[END - BEG -1];

void AssignArray()
{
    commands[REGISTER - 1].assign("REGISTER");
    commandHelp[REGISTER -1].assign("Registers an application with the process monitor thread\n"
                                        "Please set the appname and other values after adding the"
                                        "application\n"
                                        "register <APP KEY>\n");

    commands[UNREGISTER - 1].assign("UNREGISTER");
    commandHelp[UNREGISTER - 1].assign("Unregister an application with the process monitor daemon\n"
                                            "unregister <APP KEY>\n");

    commands[SET_PROPERTY - 1].assign("SET_PROPERTY");
    commandHelp[SET_PROPERTY - 1].assign("Sets the property of a registered application\n"
                                            "set_property <APP KEY> <Property> <value>\n"
                                            "Possible Properties <name, location, argc, argv, priority, loglocation>\n");

    commands[START_MONITOR - 1].assign("START_MONITOR");
    commandHelp[START_MONITOR - 1].assign("Starts the monitor of the application specified\n");

    commands[STATUS - 1].assign("STATUS");
    commandHelp[STATUS - 1].assign("Shows the status of the requested app\n"
                                            "status <APP KEY>\n");

    commands[LIST - 1].assign("LIST");
    commandHelp[LIST - 1].assign("Lists all the apps registered with process monitor daemon\n"
                                            "list"
                                            "list\n");
    
    commands[HELP - 1].assign("HELP");
    commandHelp[HELP - 1].assign("Displays the help\n");

    commands[SAVE_TO_FILE - 1].assign("SAVE_TO_FILE");
    commandHelp[SAVE_TO_FILE - 1].assign("Saves the changes made using client to the config file\n");

    commands[EXIT - 1].assign("EXIT");
    commandHelp[EXIT - 1].assign("Exits the client\n");
}

int CheckValidityOfCommand(string input)
{
    string temp;
    int valid = 0, i;
    temp = input.substr(0, input.find(' '));
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

bool CheckValidArgs(int command, string input)
{
    //TODO check valid arg No and type
    return true;
}

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

bool GetDaemonResponse(string & response, int length = 100)
{
    char * responseArray = new char[length];
    int ret = -1;
    bool readFailed = false;
    ret = read(clientDesc, responseArray, length);
    if (ret <= 0)
    {
        readFailed = true;
    }
    else
    {
        response.assign(responseArray);
    }
    delete responseArray;
    return readFailed;
}

void DisplayHelp()
{
    int i = 0;
    for (i = 1;i < END - BEG -1; i++)
    {
        cout << commands[i - 1] << endl;
        cout << commandHelp[i - 1] << endl;
    }
}

void ConvertToUpperCase(string & input)
{
    int i = 0;
    for (i = 0;i < input.length(); i++)
    {
        if (input.at(i) >= 97 || input.at(i) <= 122)
        {
            input.at(i) -= 32;
        }
    }
}

int main()
{
    AssignArray();
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
        cout << ">";
        input.assign("");
        getline(std::cin, input,'\n');
        ConvertToUpperCase(input);
        commandId = CheckValidityOfCommand(input);
        if (input.empty())  continue;
        if (-1 == commandId)
        {
            //TODO Give suggestions to the user if command is Invalid
            cout << endl << "Invalid Command" << endl;
            continue;
        }
        if (CheckValidArgs(commandId, input))
        {
            if (HELP -1 == commandId)
            {
                DisplayHelp();
                continue;
            }
            if (SendCommandToDaemon(input))
            {
                if (EXIT -1 == commandId)
                {
                    exit(EXIT_SUCCESS);
                }

                if (GetDaemonResponse(response))
                { 
                    cout << "Failed to Get Response for command " << input << response << endl;
                    continue;
                }
                else
                {
                    cout << response;
                }
            }
            else
            {
                cout << "Sending Data to daemon failed " << endl;
                continue;
            }
        }
        else
        {
            cout << "Invalid Args" << endl;
            cout << commandHelp[commandId] << endl;
            continue;
        }
        cout << "\n";
    }

    return 0;
}
