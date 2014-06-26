#!/usr/bin/env python -u


import argparse
import os
from subprocess import Popen, PIPE
from threading import Thread
from Queue import Queue, Empty
import sys
import time

archiveName = 'nachos-csci402.gz'
nachosDirectory = 'nachos-csci402/'


class ExecutionContext():
    def __init__(self, fail_on_error=True):
        self.fail_on_error = fail_on_error
        self.io_q = Queue()

    def __stream_watcher(self, identifier, stream):
        for line in stream:
            self.io_q.put((identifier, line))

        if not stream.closed:
            stream.close()

    def __printer(self, process):
        while True:
            try:
                # block for 1 second
                item = self.io_q.get(True, 1)
            except Empty:
                # no output in either streams for a second. Are we done?
                if process.poll() is not None:
                    break
            else:
                identifier, line = item
                print identifier + ':', line.rstrip('\n')

    def execute(self, command):
        expected_return = 0
        fail_on_error = self.fail_on_error

        process = Popen(command, stdout=PIPE, stderr=PIPE, shell=True)
        Thread(target=self.__stream_watcher, name='stdout-watcher', args=('STDOUT', process.stdout)).start()
        Thread(target=self.__stream_watcher, name='stderr-watcher', args=('STDERR', process.stderr)).start()
        Thread(target=self.__printer, name='printer', args=(process,)).start()

        process.wait()
        return_code = process.returncode

        if return_code is not expected_return and fail_on_error:
            raise IOException("Error executing command")

        return return_code

def where(program):

    def is_exe(fpath):
        return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

    fpath, fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            path = path.strip('"')
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file

    return None


def init(inParam):
    """Prime the system by downloading pip (if necessary) and all package dependencies"""

    print "The following will be installed on your system\n"

    pipPresent = where('pip') is not None

    if(pipPresent is False):
        print "PIP: The python package manager\n"


    print "Python Packages:"
    print "\twatchdog"
    
    c = (raw_input("continue(y|n): ")[0]).lower()
    
    if(c == "n"):
        sys.exit(0)

    print "The rest of the script needs to run under sudo to continue.\nPlease enter your system password when prompted."
    exe = ExecutionContext(True)
    exe.execute('sudo cd ./')
    
    if(pipPresent is False):
        print "Installing PIP"
        print "=================="
        exe.execute('curl https://raw.github.com/pypa/pip/master/contrib/get-pip.py --output "get-pip.py"')
        exe.execute('sudo python get-pip.py')

    print "Installing watchdog"
    print "========================="
    exe.execute('sudo ARCHFLAGS=-Wno-error=unused-command-line-argument-hard-error-in-future pip install watchdog')

    print "Finished"
    sys.exit(0)


def pull(remoteWorkingDirectory):
    exe = ExecutionContext(True)

    print "Downloading "+nachosDirectory+"from aludra:"+remoteWorkingDirectory
    print "=================================================================================="

    print "compressing remote directory"
    exe.execute('ssh aludra "cd '+remoteWorkingDirectory+'; tar -cf --exclude=*.o '+archiveName+' '+nachosDirectory+'"')

    print "downloading remote directory"
    exe.execute('scp aludra:'+remoteWorkingDirectory+'/'+archiveName+' ./')
    exe.execute('tar -xf '+archiveName)

    print "cleaning up"
    exe.execute('ssh aludra "cd '+remoteWorkingDirectory+'; rm -f '+archiveName+'"')
    exe.execute('rm '+archiveName)


def push(remoteWorkingDirectory):
    exe = ExecutionContext(True)
    print "syncing changes with aludra"
    exe.execute('rsync -rtvz -C --delete --include-from=includes.txt --exclude-from=excludes.txt * aludra:'+remoteWorkingDirectory)

def watch(remoteWorkingDirectory):

    try:
        from watchdog.events import PatternMatchingEventHandler
        from watchdog.observers import Observer
    except ImportError, e:
        print "missing script dependencies"
        print "run python kolsh.py init to resolve"
        sys.exit(1)

    class FSEventHandler(PatternMatchingEventHandler):
        def __init__(self):
            ignore_patterns = [".git", "*.s", "bin"]
            patterns = ["*"]
            super(FSEventHandler, self).__init__(ignore_patterns=ignore_patterns, patterns=patterns)

        def process(self, event):
            # the file will be processed here
            print event.src_path, event.event_type      # print now only for debug
            push(remoteWorkingDirectory)

        def on_any_event(self, event):
            self.process(event)


    path = './'+nachosDirectory
    observer = Observer()
    observer.schedule(FSEventHandler(), path, recursive=True)
    observer.start()

    print "watching the current directory for changes..."

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        observer.stop()

    observer.join()


def help(inParam):
    parser.print_help()

actions = {
    "init"  : init,
    "pull"  : pull,
    "push"  : push,
    "watch" : watch,
    "help"  : help
}


parser = argparse.ArgumentParser(description='A utility that assists with working with Nachos on Aludra.')
parser.add_argument('action', metavar='A', type=str, nargs='?', default='help', help='An action command. Can be any of [init/push/pull/watch/help]')
parser.add_argument('remote_working_directory', metavar='R', type=str, nargs='?', default=None, help='Remote working directory')
parser.add_argument('-e', '--env', dest='read_remote_working_dir_from_env', action='store_true', help='Read the remote working directory from the current environment.')

def main():
    args = vars(parser.parse_args())
    action = args["action"]


    remoteWorkingDirectory = args["remote_working_directory"]
    if(args["read_remote_working_dir_from_env"]):
        try:
            remoteWorkingDirectory = os.environ['KOLSH_REMOTE_WORKING_DIR']
        except KeyError as ke:
            print "Error: Cannot read remote working directory from the environment"
            print "The environment variable KOLSH_REMOTE_WORKING_DIR is not set"
            sys.exit(1)

    performRWDCheck = action != "help" and action != "init"

    if(performRWDCheck and remoteWorkingDirectory is None):
            print action is not "help" and action is not "init"
            print "Error: A remote working directory must be specified"
            print "see kolsh --help"
            sys.exit(2)

    actions[action](remoteWorkingDirectory)


if __name__ == '__main__':
    main()