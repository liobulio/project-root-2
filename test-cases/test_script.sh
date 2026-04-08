#!/bin/bash

Shell_program="./mysh"

for file in *.txt
do
        if [[ "$file" == *"_result.txt" ]]
        then
		continue
        fi

        base_name=$(basename "$file" .txt) #basename removes the path name
                                           #and removes .txt
        result_file="${base_name}_result.txt"

        if [ ! -f "$result_file" ]
        then
                echo "result_file not found"
                continue
        fi

        $Shell_program < $file > output.tmp # feed the test file
                                            # write into output file

        diff -w output.tmp "$result_file" > /dev/null

        if [ $? -eq 0 ]
        then
                echo "pass $file"

        else
                echo "fail $file"
		echo "--------------------------------"
        	# Print the side-by-side difference
        	diff -y --suppress-common-lines output.tmp "$result_file"
        	echo "--------------------------------"
        fi
done

rm -f my_output.temp


