#include <fstream>
#include <iostream>
#include <string>

#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

bool CheckAppName(string & name, const string searchApp)
{
    DIR * pDir;
    struct direct * pCont;
    struct stat filetype;
    string filename;
    filename = name + "/comm";
    ifstream ifile;

    if (stat(filename.c_str(), &filetype) == 0)
    {
        ifile.open(filename.c_str());
        if (ifile.good())
        {
            string appName;
            getline(ifile, appName);
            if (0 == appName.compare(searchApp))
            {
                cout << "Found the PID of App " << searchApp << endl;
                return true;
            }
            ifile.close();
        }
        else
        {
            cout << "Failed to open the file" << filename << endl; 
        }
    }
    return false;
}

bool ParseProc(string searchApp, string & pid)
{
    DIR * pDir;
    struct dirent * pDirCont;
    string filename("");
    bool found = false;

    pDir = opendir("/proc/");
    if (pDir == NULL)
    {
        cout << "Failed to open /proc/ " << endl;
        return 0;
    }
    else
    {
        while ( (pDirCont = readdir(pDir)) != 0 )
        {
            struct stat filetype;
            filename = "";
            filename = filename + "/proc/" + pDirCont->d_name;
            if ( -1 == stat(filename.c_str(), &filetype))
            {
                continue;
            }
            if ( S_ISDIR( filetype.st_mode ))
            {
                if ( CheckAppName(filename, searchApp ))
                {
                    cout << "The pid of " << searchApp << " is  " << pDirCont->d_name << endl;
                    found = true;
                    pid.assign(pDirCont->d_name);
                    break;
                }
            }
        }
        /*if (!found)
        {
            cout << "Failed to find PID of " << searchApp << endl;
        }*/
    }
    closedir(pDir);
    
    return found;
}
