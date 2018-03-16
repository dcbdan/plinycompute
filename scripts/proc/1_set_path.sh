#!/bin/bash
#  Copyright 2018 Rice University                                           
#                                                                           
#  Licensed under the Apache License, Version 2.0 (the "License");          
#  you may not use this file except in compliance with the License.         
#  You may obtain a copy of the License at                                  
#                                                                           
#      http://www.apache.org/licenses/LICENSE-2.0                           
#                                                                           
#  Unless required by applicable law or agreed to in writing, software      
#  distributed under the License is distributed on an "AS IS" BASIS,        
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
#  See the License for the specific language governing permissions and      
#  limitations under the License.                                           
#  ======================================================================== 
# By Jia

#local_b_dir=$PDB_HOME/scripts
#remote_b_dir=$PDB_HOME/scripts
local_c_dir=$PDB_HOME/scripts/proc
ip_len_valid=3
pem_file=$1
user=$2

echo "-------------step1: distribute the shell programs"
arr=($(awk '{print $0}' $PDB_HOME/conf/serverlist))
length=${#arr[@]}
for (( i=0 ; i<=$length ; i++ ))
do
        ip_addr=${arr[i]}
	if [ ${#ip_addr} -gt "$ip_len_valid" ]
	then
		echo -e "\n+++++++++++ collect info for server: $ip_addr +++++++++++"
                ssh -i $pem_file $user@$ip_addr 'mkdir ~/pdb_temp' #todo
                #todo: also copy the installation directory to each node
                scp -i $pem_file -r $local_c_dir/local_sys_collect.sh $user@$ip_addr:~/pdb_temp/
	fi
done



