#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import time

import os

from util import util

__author__ = "Marc-Andre Vef"
__email__ = "vef@uni-mainz.de"

global PRETEND
global PSSH_PATH
global WAITTIME


def check_dependencies():
    global PSSH_PATH
    """Check if pssh is installed"""
    pssh_path = os.popen('which pssh').read().strip()
    if pssh_path != '':
        PSSH_PATH = pssh_path
        return
    pssh_path = os.popen('which parallel-ssh').read().strip()
    if pssh_path != '':
        PSSH_PATH = pssh_path
        return
    print '[ERR] parallel-ssh/pssh executable cannot be found. Please add it to the parameter list'
    exit(1)


def shutdown_system(daemon_path, nodelist, sigkill):
    """Shuts down ADAFS on specified nodes.

    Args:
        daemon_path (str): Path to daemon executable
        nodelist (str): Comma-separated list of nodes where adafs is launched on
        sigkill (bool): If true force kills daemons
    """
    global PSSH_PATH
    global PRETEND
    global WAITTIME
    # get absolute paths
    daemon_path = os.path.realpath(os.path.expanduser(daemon_path))
    pssh_nodelist = ''
    if not os.path.exists(daemon_path) or not os.path.isfile(daemon_path):
        print '[ERR] Daemon executable not found or not a file'
        exit(1)
    nodefile = False
    if os.path.exists(nodelist):
        nodefile = True  # TODO
        print 'Nodefiles are not supported yet'
    if PSSH_PATH is '':
        check_dependencies()
    # set pssh arguments
    pssh = '%s -O StrictHostKeyChecking=no -i -H "%s"' % (PSSH_PATH, nodelist.replace(',', ' '))
    if sigkill:
        cmd_str = '%s "pkill -f -SIGKILL \"%s\""' % (pssh, daemon_path)
    else:
        cmd_str = '%s "pkill -f -SIGTERM \"%s\""' % (pssh, daemon_path)
    if PRETEND:
        print 'Pretending: %s' % cmd_str
    else:
        print 'Running: %s' % cmd_str
        pssh_ret = util.exec_shell(cmd_str, True)
        err = False
        for line in pssh_ret:
            if 'FAILURE' in line.strip()[:30]:
                err = True
                print '------------------------- ERROR pssh -- Host "%s" -------------------------' % \
                      (line[line.find('FAILURE'):].strip().split(' ')[1])
                print line
        if not err:
            if sigkill:
                print 'pssh daemon launch successfully executed. FS daemons have been force killed ...'
                exit(1)
            else:
                print 'pssh daemon launch successfully executed. Checking for FS shutdown errors ...\n'
        else:
            print '[ERR] with pssh. Aborting...'
            exit(1)

    if not PRETEND:
        print 'Give it some time (%d second) to finish up ...' % WAITTIME
        for i in range(WAITTIME):
            print '%d\r' % (WAITTIME - i),
            time.sleep(1)
    print 'Checking logs ...\n'

    cmd_chk_str = '%s "tail -4 /tmp/adafs_daemon.log"' % pssh
    if PRETEND:
        print 'Pretending: %s' % cmd_chk_str
    else:
        print 'Running: %s' % cmd_chk_str
        pssh_ret = util.exec_shell(cmd_chk_str, True)
        err = False
        fs_err = False
        for line in pssh_ret:
            if line == '':
                continue
            if 'Failure' in line.strip()[:30]:
                err = True
                print '------------------------- ERROR pssh -- Host "%s" -------------------------' % \
                      (line[line.find('FAILURE'):].strip().split(' ')[1])
                print line
            else:
                # check for errors in log
                if not 'All services shut down.' in line[line.strip().find('\n') + 1:]:
                    fs_err = True
                    print '------------------------- WARN pssh -- Host "%s" -------------------------' % \
                          (line.strip().split(' ')[3].split('\n')[0])
                    print '%s' % line[line.find('\n') + 1:]

        if not err and not fs_err:
            print 'pssh logging check successfully executed. Looks prime.'
        else:
            print '[WARN] while checking fs logs. Something might went wrong when shutting down'
            exit(1)


if __name__ == "__main__":
    # Init parser
    parser = argparse.ArgumentParser(description='This script stops adafs on multiple nodes',
                                     formatter_class=argparse.RawTextHelpFormatter)
    # positional arguments
    parser.add_argument('daemonpath', type=str,
                        help='path to the daemon executable')
    parser.add_argument('nodelist', type=str,
                        help='''list of nodes where the file system is launched. This can be a comma-separated list
                             or a path to a nodefile (one node per line)''')

    # optional arguments
    parser.add_argument('-p', '--pretend', action='store_true',
                        help='Output adafs launch command and do not actually execute it')
    parser.add_argument('-9', '--sigkill', action='store_true',
                        help='Force kill adafs_daemons')
    parser.add_argument('-P', '--pssh', metavar='<PSSH_PATH>', type=str, default='',
                        help='Path to parallel-ssh/pssh. Defaults to /usr/bin/{parallel-ssh,pssh}')
    args = parser.parse_args()

    if args.pretend is True:
        PRETEND = True
    else:
        PRETEND = False
    PSSH_PATH = args.pssh
    WAITTIME = 5
    shutdown_system(args.daemonpath, args.nodelist, args.sigkill)

    print '\nNothing left to do; exiting. :)'
