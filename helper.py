import re

def getstatusoutput(cmd):
    """Return (status, output) of executing cmd in a shell."""

    import sys
    import os
    pipe = os.popen(cmd, 'r')
    text = pipe.read()
    sts = pipe.close()
    if sts is None: sts = 0
    if text[-1:] == '\n': text = text[:-1]
    return sts, text

def git_version_string():
    # first try to get some meaningfull string
    cmd_git = getstatusoutput('git describe --tags --always 2>nul')
    if cmd_git[0] or cmd_git[1] == '':
        # git not available/not a git repository/empty repository
        return ''
    return cmd_git[1]

def git_version_to_numeric_version(s):
    # v0.1.2[-3-...] -> 0,1,2,3
    num_ver = "1, 0, 0, 0"
    m = re.match(r'v(\d+)\.(\d+)\.(\d+)(?:-(\d+).*)?', s)
    if (m):
        v1, v2, v3, v4 = m.group(1), m.group(2), m.group(3), m.group(4)
        if not v4: v4 = "0"
        num_ver = "%s, %s, %s, %s" % (v1, v2, v3, v4)
    return num_ver

        
