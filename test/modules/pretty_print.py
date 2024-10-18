import sys

# This functions use ANSII escape code to control the terminal
# https://en.wikipedia.org/wiki/ANSI_escape_code

def bold( s ):
    if type(s) is str:
        s = bytes(s, 'utf-8')
    res =  b"\x1B\x5B1m" + s + b"\x1B\x5Bm"
    return res

def green(s ):
    if type(s) is str:
        s = bytes(s, 'utf-8')
    res =  b"\x1B\x5B32m" + s + b"\x1B\x5Bm"
    return res

def red(s ):
    if type(s) is str:
        s = bytes(s, 'utf-8')
        res =  b"\x1B\x5B31m" + s + b"\x1B\x5Bm"
        return res

def yellow(s ):
    if type(s) is str:
        s = bytes(s, 'utf-8')
        res =  b"\x1B\x5B33m" + s + b"\x1B\x5Bm"
        return res
    
def print_okk():
    green("OKK")

def print_err():
    red("ERR")

def color_print(s, end=b"\n"):
    sys.stdout.buffer.write(s)
    sys.stdout.buffer.write(end)
    flush()


def flush():
    sys.stdout.flush()
