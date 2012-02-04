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
