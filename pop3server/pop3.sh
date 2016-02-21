#!/bin/bash

pid_num=0

start()
{
	if [ $pid_num -gt 0 ]
	then
	echo "the pop3 server is already started!"
	else
	   echo -n "Start............................................."
       echo -n "["
       echo -ne "\33[32m"
       echo -n "OK"
       echo -ne "\33[0m"
       echo "]"
	./pop3 &
	fi
}


stop()
{
    if [ $pid_num -eq 0 ]
    then 
       echo "No process started. Stop failed"
    else
       for pid in `pidof pop3`
       do
          kill -3 $pid
          kill -9 $pid
       done
       pid_num=0;
       echo -n "Stop.............................................."
       echo -n "["
       echo -ne "\33[32m"  #32m设置字体背景色
       echo -n "OK"
       echo -ne "\33[0m"  
       echo "]"
    fi
}

restart ()
{
	if [ $pid_num -gt 0 ]
	then
   	stop
	fi
   	start
}

if [ $# -ne 1 ]
then
echo "please input like ./pop3 start or stop or restart"
else
pid_num=`pidof pop3 | wc -w`
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
esac
fi







