#!/bin/bash
#!/bin/bash

:<<!
this is annotate
this is annotate
this is annotate
!
:<<EOF
this is also annotate
this is also annotate
this is also annotate
EOF

echo "========================="
mywebsite="www"
echo ${mywebsite}.gg.com

#readonly myname
myname="name"
echo "$myname after readonly"

unset myname
echo "\$myname $myname is unsetted."

echo "========================="
hisname="poOlOoq"
echo "${hisname}'s length is ${#hisname}"
echo "${hisname}'s substr: ${hisname:3:3}"
echo "${hisname}'s substr: ${hisname:3}"
echo index of l is `expr index ${hisname} l`


echo "==========================="
myarray=(arr1 arr2 arr3)
echo "myarray has ${#myarray[@]} elements:"
for arr in ${myarray[@]}
    do
        echo $arr
    done
    echo "or..."
    for arr in ${myarray[*]}
    do
        echo $arr
    done
echo "length of myarray[1] is ${#myarray[1]}"

echo "========================="
echo "the script is running in process $$"
echo "the number of args is $#. the args are $*"
echo the name of this script is $0
if [ -n $1 ];then
    echo "the 1st arg of this script is $1"
fi
if [ ! -z $2 ];then #there must be a space after "[", before "]", after "!"
    echo "the 2nd arg of this script is $2"
fi
if [ -z $3 ];then #there must be a space after "[", before "]", after "!"
    echo "the 3th arg of this script is empty"
fi

echo "========================="
if ZAGL=$(ls ./ZAGL* 2>/dev/null);then
    L6GQ=$(echo $ZAGL | sed -r 's/ZAGLAL/L6GQAG/g')
    sed -r 's/ZAGLAL/L6GQAG/g' $ZAGL> tmp
    #sed -r 's/ZAGL/L6GQ/g' $ZAGL > tmp
    mv tmp $L6GQ
fi


