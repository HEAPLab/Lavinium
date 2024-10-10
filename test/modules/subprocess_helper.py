import subprocess

from modules.pretty_print import flush
import modules.debug as debug


def run(shell_string : str, capture_stdout = False):
    if debug.DEBUG:
        print(shell_string)
    p_output = subprocess.PIPE if capture_stdout else subprocess.DEVNULL
    p_err = subprocess.PIPE
    s = subprocess.run(shell_string, shell=True , stdout=p_output, stderr=p_err)
    if capture_stdout:
        return (s.returncode, s.stdout, s.stderr)        
    else:
        return (s.returncode, None, s.stderr)        
        

