#!/bin/bash

pid_num=0

start()
{
   if [ $pid_num -gt 0 ]
   then 
       echo "The process had already started"
   else
       echo -n "Start............................................."
       echo -n "["
       echo -ne "\33[32m"
       echo -n "OK"
       echo -ne "\33[0m"
       echo "]"
       ./smtp &
   fi 
}

stop()
{
    if [ $pid_num -eq 0 ]
    then 
       echo "No process started. Stop failed"
    else
       for pid in `pidof smtp`
       do
          kill -3 $pid
          kill -9 $pid
       done
       pid_num=0;
       echo -n "Stop.............................................."
       echo -n "["
       echo -ne "\33[32m"
       echo -n "OK"
       echo -ne "\33[0m"
       echo "]"
    fi
}

restart()
{
   if [ $pid_num -gt 0 ]
   then
       stop
   fi
    start
}

stat()
{
        if [ $pid_num -gt 0 ]
        then 
        echo "the pop3 server is running!"
        else
        echo "the pop3 server is stopped!"
        fi
}

# //命令行的参数个数不等于1
if [ $# -ne 1 ] 
then
    echo "Please input the option: start/stop/restart"
else
    pid_num=`pidof smtp | wc -w`
    case $1 in 
        start)
         start
         ;;
        stop)
         stop
         ;;
        restart)
         restart
         ;;
         stat)
                stat
                ;;
    esac
fi
