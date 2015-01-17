#!/usr/bin/env python3
# Just need python >=2.5
import sys, os.path, re

def error(message):
	sys.stderr.write(message + '\n')
	sys.exit(1)

def main(argv):
	if len(argv)!=(3+1):
		error("usage: %s strace_log target regexp" % os.path.basename(argv[0]))
	log_fname = argv[1]
	target = argv[2]
	fname_match = re.compile(argv[3])
	
	init_read_files = set()
	ever_write_files = set()
	
	pid_strip_stderr  = re.compile("\[pid ([0-9]+)\]\s*(.+)", re.DOTALL)
	pid_strip_outfile  = re.compile("([0-9]+)\s*(.+)", re.DOTALL)
	pid_strip = pid_strip_outfile
	open_parse = re.compile("open\(\"(.+)\", (.+)\)\s*=\s*(-?[0-9]+)\s*.*", re.DOTALL)
	read_parse = re.compile("read\(([0-9]+), \".*\"\\.*, [0-9]+\)\s*=\s*([0-9]+)", re.DOTALL)
	write_parse= re.compile("write\(([0-9]+), \".*\"\\.*, [0-9]+\)\s*=\s*([0-9]+)", re.DOTALL)
	
	
	fd_fn_table = {}
	
	with open(log_fname, 'r') as f:
		for line in f:
			pid = ""
			mat = pid_strip.match(line)
			if (mat):
				#child process
				pid = mat.group(1)
				line = mat.group(2)
					
			if line.startswith("open"):
				mat = open_parse.match(line)
				if(mat):
					fd = int(mat.group(3))
					if(fd>=0):
						fd = pid + "-" + str(fd)
						fn = mat.group(1)
						fn = os.path.abspath(fn)
						if fd in fd_fn_table:
							del fd_fn_table[fd]
						if fname_match.search(fn) and mat.group(2).find("O_DIRECTORY")==-1:
							fd_fn_table[fd] = fn
				else:
					print("Should've matched (open_parse): " +line)
				continue
					
			if line.startswith("read"):
				mat = read_parse.match(line)
				if(mat):
					fd = pid + "-" + mat.group(1)
					ret = int(mat.group(2))
					if fd in fd_fn_table:
						fn = fd_fn_table[fd]
						if((fn not in init_read_files) and (fn not in ever_write_files) and ret>0):
							init_read_files.add(fn)
				else:
					print("Should've matched (read_parse): " +line)
				continue
					
			if line.startswith("write"):
				mat = write_parse.match(line)
				if(mat):
					fd = pid + "-" + mat.group(1)
					if fd in fd_fn_table:
						ret = int(mat.group(2))
						fn = fd_fn_table[fd]
						if (fn not in ever_write_files) and ret>0:
							ever_write_files.add(fn)
				else:
					print("Should've matched (write_parse): " +line)
				continue
				
			#print("Unused logline: " + line)
	
	#remove if the files ultimately were deleted. Sets become lists
	init_read_files = [fname for fname in init_read_files if os.path.isfile(fname)]
	ever_write_files = [fname for fname in ever_write_files if os.path.isfile(fname)]
	
	#Output the information
	with open(target + ".dep", "w") as dep_file:
		if len(init_read_files)>0:
			dep_file.write(target + ' :')
			for fname in init_read_files:
				fname_rel = os.path.relpath(fname)
				if not fname.startswith(".."):
					fname = fname_rel
				dep_file.write(' ' + fname)
			dep_file.write('\n')
			
		if len(ever_write_files)>0:
			for fname in ever_write_files:
				fname_rel = os.path.relpath(fname)
				if not fname.startswith(".."):
					fname = fname_rel
				dep_file.write(fname + ' ')
			dep_file.write(' : ' + target + '\n')

if  __name__ =='__main__':
	main(sys.argv)
	