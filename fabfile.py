from fabric.api import *
from fabric.contrib.project import rsync_project
import os
import sys

try:
    fw_home = os.environ['FABRICW_HOME']
except KeyError:
    print ("you need to set FABRICW_HOME in your .bashrc")
    raise
sys.path.insert(0, fw_home)
from fabconf import *

# Config!
ENV_ROOT = ""

# Initial
env.home = "/storage-home/k/ksl3"
env.user = "ksl3"
SERVER_HOST = "ssh.clear.rice.edu"
env.hosts = [SERVER_HOST]
REMOTE_DIR = "~/school/comp421/labs/lab2/"
LOCAL_DIR = ""

def initialize():
    pass

def deploy():
    push()
    with cd(REMOTE_DIR + "src/"):
        run("make clean && make")

def test():
    push()
    with cd(REMOTE_DIR + "philosophers/"):
        run("make clean && make")


def push():
    global RSYNC_EXCLUDE
    extra_opts = "--delete"
    rsync_project(
        remote_dir = REMOTE_DIR,
        local_dir = ".",
        exclude = RSYNC_EXCLUDE,
        delete = False,
        extra_opts = extra_opts
    )

def pull():
    local( "rsync -azv ksl3@ssh.clear.rice.edu:%s ." % REMOTE_DIR)

def cleanup():
    pass
