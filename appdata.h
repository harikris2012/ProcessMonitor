#include <string>
#include <iostream>
#define MAX_ARGS 20
class AppsData;
void ProcessMonitorThread(AppsData * );

/*
    Class AppData stores the data on Processess
*/
class AppsData
{
    public:

    std::string m_appID;
    std::string m_appName;
    std::string m_appLocation;
    std::string m_inputArgs;
    std::string m_logLocation;
    unsigned int m_noOfArgs;
    unsigned int m_RealNoOfArgs;
    unsigned int m_priority;
    unsigned int m_noOfCrashes;
    int m_pid;
    bool m_isRegistered;
    bool m_isRunning;
    int m_exitStatus;
    int m_execErrorCode;
    char * m_argsForExec[MAX_ARGS];
    std::thread m_processMonitorThread;
    std::mutex m_threadMutex;
    std::condition_variable_any m_condition;

    public:
    AppsData()
    {
        m_appLocation.assign("/bin");
        m_logLocation.assign("/etc/processd");
        m_noOfArgs = 0;
        m_inputArgs.assign("NOARGS");
        m_priority = 10;
        m_isRegistered = true;
        m_execErrorCode = -1;
        m_RealNoOfArgs = 0;
        for (int i = 0; i < MAX_ARGS; i++)
        {
            m_argsForExec[i] = NULL;
        }
        m_isRegistered = true;
        m_isRunning = true;
        m_RealNoOfArgs = 0;
    }
    AppsData(const AppsData& newData)
    {
        m_appID.assign(newData.m_appID);
        m_appName.assign(newData.m_appName);
        m_appLocation.assign(newData.m_appLocation);
        m_logLocation.assign(newData.m_logLocation);
        m_noOfArgs = newData.m_noOfArgs;
        m_inputArgs.assign(newData.m_inputArgs);
        m_priority = newData.m_priority;
        m_isRunning = newData.m_isRunning;
        m_isRegistered = newData.m_isRegistered;
        for (int i = 0; i <  MAX_ARGS; i++)
        {
            m_argsForExec[i] = NULL;
        }
        m_isRunning = newData.m_isRunning;
        m_pid = newData.m_pid;
        m_RealNoOfArgs = newData.m_RealNoOfArgs;
    }
    ~AppsData()
    {
        //StopThread();
        if (false == m_isRegistered)
        {
            m_processMonitorThread.join();
        }
        for (int i = 0; i <= m_RealNoOfArgs ; i++)
        {
            if (NULL != m_argsForExec[i])
            {
                free(m_argsForExec[i]);
            }
        }
    }

    
    bool StartThread()
    {
        m_processMonitorThread = std::thread(&ProcessMonitorThread, this);
    }

    void StopThread()
    {
        m_isRegistered = false;
        //m_isRunning = false;
    }
    
    bool IsStopped()
    {
        return !m_isRegistered;
    }

    bool ProcessInputArgs()
    {
        log_info("converting input argument to the format needed");
        std::string temp;
        temp.assign(m_appLocation);
        if (temp.back() != '/')
        {
            temp.append(1, '/');
        }
        temp.append(m_appName);
        m_argsForExec[0] = strdup(temp.c_str());
        if (NULL == m_argsForExec[0])
        {
            log_error("Failed to allocate memory exiting");
            return false;
        }
        int searchPos = 0, searchPosBeg = 0;
        int currentArg = 1;
        if (0 == m_inputArgs.compare("NOARGS"))
        {
            log_info("No arguments so no need of processing");
            return true;
        }
        while (std::string::npos != (searchPos = m_inputArgs.find(' ', searchPos)))
        {
            std::string temp = m_inputArgs.substr(searchPosBeg, searchPos - searchPosBeg);
            //cout << temp << "   " << searchPos  << "  " << searchPosBeg << " " << currentArg << endl;
            if (temp.at(0) == '"')
            {
                log_debug("Input args have \" in them parsing the input args");
                int index = searchPos;
                bool parseCorrect = false;
                while (1)
                {
                    index = m_inputArgs.find(' ', index);
                    if (std::string::npos == index)
                    {
                        if (m_inputArgs.at(m_inputArgs.length() - 1) == '"')
                        {
                            parseCorrect = true;
                            log_debug("Found the end of \"");
                            break;
                        }
                    }
                    else
                    {
                        if (m_inputArgs.at(index - 1) == '"')
                        {
                            parseCorrect = true;
                            log_debug("Found the end of \"");
                            break;
                        }
                        else
                        {
                            index++;
                            continue;
                        }
                    }
                }
                if (false == parseCorrect)
                {
                    log_error("Error in the Input arguments cannot find end of \"");
                    return false;
                }
                else
                {
                    if (std::string::npos != index)
                    {
                        /*m_argsForExec[currentArg] = strdup(m_inputArgs.substr(searchPosBeg, index - searchPosBeg).c_str());
                        if (NULL == m_argsForExec[currentArg])
                        {
                            log_error("Failed to allocate memory");
                            return false;
                        }
                        currentArg++;*/
                        searchPos = index;
                    }
                    else
                    {
                        /*m_argsForExec[currentArg] = strdup(m_inputArgs.substr(searchPosBeg).c_str());
                        if (NULL == m_argsForExec[currentArg])
                        {
                            log_error("Failed to allocate memory");
                            return false;
                        }
                        log_debug("Finished parsing the args");*/
                        break;
                    }
                }
            }
            temp = m_inputArgs.substr(searchPosBeg, searchPos - searchPosBeg);
            m_argsForExec[currentArg] = strdup(temp.c_str());
            if (NULL == m_argsForExec[currentArg])
            {
                log_error("Failed to allocate memory exiting");
                return false;
            }
            searchPos++;
            searchPosBeg = searchPos;
            currentArg++;
        }
        m_argsForExec[currentArg] = strdup(m_inputArgs.substr(searchPosBeg).c_str());
        if (NULL == m_argsForExec[currentArg])
        {
            log_error("Failed to allocate memory exiting");
            return false;
        }
        m_RealNoOfArgs = currentArg;
        if (m_RealNoOfArgs > m_noOfArgs)
        {
            log_error("Actual no of arguments is greater than argv, trying to start with " << m_noOfArgs);
            return false;
        }
    }
   void ShowClassValues()
    {
        std::cout << "-------------------------------------" << std::endl;
        std::cout << "Id: " << m_appID << std::endl;
        std::cout << "Name: " << m_appName << std::endl;
        std::cout << "Location: " << m_appLocation << std::endl;
        std::cout << "Priority: " << m_priority << std::endl;
        std::cout << "Log Location: " << m_logLocation << std::endl;
        std::cout << "Input Args: " << m_inputArgs << std::endl;
        std::cout << "No of Args: " << m_noOfArgs << std::endl;
        std::cout << "Splitted Args are " << std::endl;
        for (int i = 0; i <= m_noOfArgs; i++)
        {
            if (NULL != m_argsForExec[i])
            {
                std::cout << m_argsForExec[i] << "<>" << std::endl;
            }
        }
        std::cout << "-------------------------------------" << std::endl;
    }
};
