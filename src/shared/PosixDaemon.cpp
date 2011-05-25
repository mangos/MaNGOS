/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "Config/Config.h"
#include "PosixDaemon.h"
#include <cstdio>
#include <iostream>
#include <fstream>

pid_t parent_pid = 0, sid = 0;

void daemonSignal(int s)
{

    if (getpid() != parent_pid)
    {
        return;
    }

    if (s == SIGUSR1)
    {
        exit(EXIT_SUCCESS);
    }

    if (sid) {
        kill(sid, s);
    }

    exit(EXIT_FAILURE);
}


void startDaemon(uint32_t timeout)
{
    parent_pid = getpid();
    pid_t pid;

    signal(SIGUSR1, daemonSignal);
    signal(SIGINT, daemonSignal);
    signal(SIGTERM, daemonSignal);
    signal(SIGALRM, daemonSignal);

    sid = pid = fork();

    if (pid < 0) {
      exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        alarm(timeout);
        pause();
        exit(EXIT_FAILURE);
    }

    umask(0);

    sid = setsid();

    if (sid < 0) {
      exit(EXIT_FAILURE);
    }

    if ((chdir("/")) < 0) {
      exit(EXIT_FAILURE);
    }

    freopen("/dev/null", "rt", stdin);
    freopen("/dev/null", "wt", stdout);
    freopen("/dev/null", "wt", stderr);
}

void stopDaemon()
{
    std::string pidfile = sConfig.GetStringDefault("PidFile", "");
    if(!pidfile.empty())
    {
        std::fstream pf(pidfile.c_str(), std::ios::in);
        uint32_t pid = 0;
        pf >> pid;
        if (kill(pid, SIGINT) < 0)
        {
            std::cerr << "Unable to stop daemon" << std::endl;
            exit(EXIT_FAILURE);
        }
        pf.close();
    }
    else
    {
        std::cerr << "No pid file specified" << std::endl;
    }

    exit(EXIT_SUCCESS);
}

void detachDaemon()
{
    if (parent_pid)
    {
        kill(parent_pid, SIGUSR1);
    }
}


void exitDaemon()
{
    if (parent_pid && parent_pid != getpid())
    {
        kill(parent_pid, SIGTERM);
    }
}


struct WatchDog
{
    ~WatchDog()
    {
        exitDaemon();
    }
};

WatchDog dog;
