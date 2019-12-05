#!/bin/bash

min_split_size=2
input_floder=input_files
exec_floder=bin

# determine argc is a positive number
is_num() {
    if ! [[ "$1" =~ ^[1-9][0-9]*$ ]]; then
        return 0
    else
        return 1
    fi;
}

# select the implementation type
echo "Please select the following implementation type:"
echo " 1. serial."
echo " 2. parallel."
read -p "enter: " imp_type
until [ "$imp_type" = "1" -o "$imp_type" = "2" ]
do
    read -p "Input invaild, please input again: " imp_type
done
if [ "$imp_type" = "1" ]; then  # determine exexc is exist
    if [ ! -f "$exec_floder/tsp" ]; then
        echo -e "\nUse 'make tsp' to generate the exe first"
        exit -1
    fi
else
    if [ ! -f "$exec_floder/parral_tsp" ]; then
        echo -e "\nUse 'make parral_tsp' to generate the exe first"
        exit -1
    fi
fi

# select the input file
j=0
for i in `ls -1 $input_floder`
do
    if [ $j -eq 0 ]; then echo -e "\nPlease select the following input file: "; fi;
    echo "" `expr $j + 1`. $i.
    folder_list[j]=$i
    j=`expr $j + 1`
done
if [ ${#folder_list[*]} -eq 0 ]; then   # determine folder is empty
    echo -e "\nNo input file"
    exit -1
fi;
read -p "enter: " file_name_index
is_num $file_name_index
until [[ $? -eq 1 && $file_name_index -le ${#folder_list[*]} && $file_name_index -gt 0 ]]
do
    read -p "Input invaild, please input again: " file_name_index
    is_num $file_name_index
done

if [ "$imp_type" = "2" ]; then
    # enter number of threads
    echo ""
    read -p "Please enter number of threads: " thread_num
    is_num $thread_num
    until [ $? -eq 1 ]
    do
        read -p "Input invaild, please input again: " thread_num
        is_num $thread_num
    done

    # enter min split size
    echo ""
    read -p "Please enter min split size (Enter by default 2): " min_split_size
    if [ "$min_split_size" = "" ]; then
         min_split_size=2 
    fi
    is_num $min_split_size
    until [ $? -eq 1 ]
    do
        read -p "Input invaild, please input again (Enter by default 2): " min_split_size
        if [ "$min_split_size" = "" ]; then
            min_split_size=2 
        fi
        is_num $min_split_size
    done

    # execute the command
    echo -e "\nPlease wait..."
    $exec_floder/parral_tsp $thread_num $input_floder/${folder_list[`expr $file_name_index-1`]} $min_split_size
else
    # execute the command
    echo -e "\nPlease wait..."
    $exec_floder/tsp $input_floder/${folder_list[`expr $file_name_index-1`]}
fi

# if [ "$1" = "tsp" -a $# -eq 2 ]; then 
#     file_exist $2
#     bin/tsp input_files/$2
# elif [ "$1" = "parral_tsp" -a $# -ge 3 ]; then 
#     file_exist $2
#     is_num "<thread_num>" $3
#     if [ $4 ]; then
#         is_num "<min_split_size>" $4 
#         min_split_size=$4; 
#     fi;
#     bin/parral_tsp $3 input_files/$2 $min_split_size
# else
#     echo "Please usage: './run.sh tsp <input_file>' or './run.sh parral_tsp <input_file> <thread_num> <min_split_size>', <input_file> is under the input_files folder, just provide the file name without path, <min_split_size> defaults to 2."
# fi

exit 0