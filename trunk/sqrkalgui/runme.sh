#/bin/bash
### First option is 1 if we are starting and 2 if stopping. Second option 
### is argument to the program
if [ $1 -eq 1 ] ; then
	echo 'Running sqrkal daemon process with option' $2
	xterm -e "sudo sqrkald $2";
	echo 'Done'
else                   
    if [ $1 -eq 2 ] ; then
		echo 'Running sqrkal daemon process with option' $2
		skill -9 sqrkald
		iptables -F
		echo 'Done'
	fi
fi


