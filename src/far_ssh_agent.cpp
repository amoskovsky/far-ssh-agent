#include <windows.h>
#include <plugin.hpp>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <list>
#include <vector>
#include <map>
#include <stdio.h>


using namespace std;

// i18n message IDs
enum {
    M_Title
};

#define VAR_FILE ".far_ssh_agent.var"

void export_ssh_agent_vars();

static struct PluginStartupInfo info;

const wchar_t *msg(int msg_id)
{
  return info.GetMsg(info.ModuleNumber, msg_id);
}


void WINAPI _export SetStartupInfoW(const struct PluginStartupInfo* psi)
{
    info = *psi;
}

void WINAPI _export GetPluginInfoW(struct PluginInfo* pi)
{
    static const wchar_t *menu_strings[1];
    pi->StructSize = sizeof(struct PluginInfo);
    pi->Flags = PF_PRELOAD;
    menu_strings[0] = msg(M_Title);
    pi->PluginMenuStrings = menu_strings;
    pi->PluginMenuStringsNumber = sizeof(menu_strings) / sizeof(menu_strings[0]);
    export_ssh_agent_vars();
}

HANDLE WINAPI _export OpenPluginW(int OpenFrom, INT_PTR item)
{
    return  INVALID_HANDLE_VALUE;
}

string get_config_dir()
{
    char* dir = getenv("HOME");
    if (!dir)
        dir = getenv("TEMP");
    if (!dir)
        dir = ".";
    return dir; 
}

bool read_cmd_output(const string& cmd, list<string>& output) 
{
    output.clear();
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe)
        return false;
    char buffer[128];
    while(!feof(pipe)) {
        if(fgets(buffer, 128, pipe) != NULL)
            output.push_back(buffer);
    }
    _pclose(pipe);
    return true;
}

bool parse_ssh_agent_var(const string& line, string& var, string& value)
{
    //VAR=value; ....
    size_t pos1 = line.find('=');
    if (pos1 == string::npos)
        return false;
    size_t pos2 = line.find(';', pos1);
    if (pos2 == string::npos)
        return false;
    var = line.substr(0, pos1);
    value = line.substr(pos1 + 1, pos2 - pos1 - 1);
    return true;    
}

bool read_file(const string& file, list<string>& lines) 
{
    lines.clear();
    ifstream in(file.c_str());
    if (!in.is_open())
        return false;
    while (in.good() && !in.eof()) {
        string line;
        getline(in, line);
        lines.push_back(line);
    }
    return true;
}

bool process_exists(DWORD pid)
{
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
    DWORD ret = WaitForSingleObject(process, 0);
    CloseHandle(process);
    return ret == WAIT_TIMEOUT;
}

bool get_ssh_agent_vars(map<string,string>& vars) 
{
    // try to load vars for running ssh-agent from the file in user profile dir
    string var_file = get_config_dir() + "/" + VAR_FILE;
    list<string> lines;
    if (read_file(var_file, lines)) {
        vars.clear();
        for (list<string>::iterator i = lines.begin(); i != lines.end(); ++i) {
            string var, value;
            if (parse_ssh_agent_var(*i, var, value)) {
                vars[var] = value;
            }
        }
        vars["FAR_SSH_AGENT"] = "REUSED";

        if (process_exists(atol(vars["SSH_AGENT_PID"].c_str())))
            return true;
    }
    // no running ssh-agent, load a new one and save vars to file for other Far instances
    if (!read_cmd_output("ssh-agent", lines))
        return false;
    vars.clear();
    ofstream out(var_file.c_str()); // don't care if failed to open
    for (list<string>::iterator i = lines.begin(); i != lines.end(); ++i) {
        out << *i << endl;
        string var, value;
        if (parse_ssh_agent_var(*i, var, value)) {
            vars[var] = value;
        }
    }
    vars["FAR_SSH_AGENT"] = "NEW";
    return true;
}

void export_ssh_agent_vars()
{
    map<string,string> vars;     
    if (!get_ssh_agent_vars(vars))
        return;

    for (map<string,string>::iterator i = vars.begin(); i != vars.end(); ++i) {
        ::SetEnvironmentVariableA(i->first.c_str(), i->second.c_str());
    }
}

