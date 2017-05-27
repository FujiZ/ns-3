import os
import sys

program_path = sys.argv[1]
result_path = sys.argv[2]
cmd = program_path + ' --enableDS --writeResult --writeFlowInfo '
for i in range(1,9):
	os.system(cmd+'--enableC3P --miceLoad='+str(0.1*i)+' --pathOut='+result_path+'/c3p &')
	os.system(cmd+'--miceLoad='+str(0.1*i)+' --pathOut='+result_path+'/no-c3p &')
