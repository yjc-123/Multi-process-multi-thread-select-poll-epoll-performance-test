#! /bin/bash
#!/bin/bash


for((i=1;i<=1100;i=i+1))
do
	    ./a & 
	    echo "$i"   
	    sleep 0.005
    done
echo "***************************8"
cat test.log | grep "F"|wc -l 
cat test.log | grep "O"|wc -l
echo "***************************"
