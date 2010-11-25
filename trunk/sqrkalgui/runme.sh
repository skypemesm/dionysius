#/bin/bash

echo 'Running sqrkal daemon process with option' $1
xterm -e "sudo sqrkald $1";
echo 'Done'

