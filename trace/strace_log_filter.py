#!/usr/bin/env python3
# Just need python >=2.5
import sys, os.path, re

def error(message):
	sys.stderr.write(message + '\n')
	sys.exit(1)

def main(argv):
	if len(argv)!=(2+1):
		error("usage: %s strace_log dep_file" % os.path.basename(argv[0]))
	
	remove_deleted_files=1
	log_fname = argv[1]
	dep_file = argv[2]
	#env_filter = os.environ['GENDEP_FMATCH'] #this also needs search() below, not match()
	env_filter = os.getenv('GENDEP_PROJDIR', os.getcwd())
	env_filter_abs = os.path.abspath(env_filter)
	fname_filter = re.compile(env_filter_abs)
	
	# Use list instead of set to get consistent ordering
	init_read_files = []
	ever_write_files = []
	
	pid_strip_stderr  = re.compile("\[pid ([0-9]+)\]\s*(.+)", re.DOTALL)
	pid_strip_outfile  = re.compile("([0-9]+)\s*(.+)", re.DOTALL)
	pid_strip = pid_strip_outfile
	open_parse = re.compile("open\(\"(.+)\",\s+(.+)\)\s*=\s*(-?[0-9]+)\s*.*", re.DOTALL)
	read_parse = re.compile("read\(([0-9]+),\s+\".*\"\\.*, [0-9]+\)\s*=\s*([0-9]+)", re.DOTALL)
	write_parse= re.compile("write\(([0-9]+),\s+\".*\"\\.*, [0-9]+\)\s*=\s*([0-9]+)", re.DOTALL)
	dup_parse= re.compile("dup[23]?\(([0-9]+)[^\)]*\)\s*=\s*([0-9]+)", re.DOTALL)
	
	unfinished_pt1 = re.compile("(open|read|write|dup[0-9]?)(.+)<unfinished \.\.\.>$")
	unfinished_pt2 = re.compile("<\.\.\. (open|read|write|dup[0-9]?) resumed>(.+)")
	unfinished_parts = {}
	
	fd_fn_table = {}
	
	with open(log_fname, 'r') as f:
		for line in f:
			pid = ""
			mat = pid_strip.match(line)
			if (mat):
				#child process
				pid = mat.group(1)
				line = mat.group(2)
				
			#Try to piece together partial lines
			mat = unfinished_pt1.match(line)
			if(mat):
				op = mat.group(1)
				key=pid + "-" + op
				unfinished_parts[key] = op+mat.group(2)
				continue
			mat = unfinished_pt2.match(line)
			if(mat):
				op = mat.group(1)
				key=pid + "-" + op
				if key in unfinished_parts:
					line = unfinished_parts[key]+mat.group(2)
					del unfinished_parts[key]
				else:
					print("Resume line without unfinished line. key: " +key)
					continue
					
			if line.startswith("open("):
				mat = open_parse.match(line)
				if(mat):
					fd = int(mat.group(3))
					if(fd>=0):
						key = pid + "-" + str(fd)
						fn = mat.group(1)
						fn = os.path.abspath(fn)
						if key in fd_fn_table:
							del fd_fn_table[key]
						if fname_filter.match(fn) and mat.group(2).find("O_DIRECTORY")==-1:
							fd_fn_table[key] = fn
				else:
					print("Should've matched (open_parse): " +line)
				continue
					
			if line.startswith("read("):
				mat = read_parse.match(line)
				if(mat):
					fd = mat.group(1)
					key = pid + "-" + fd
					ret = int(mat.group(2))
					if key in fd_fn_table:
						fn = fd_fn_table[key]
						if((fn not in init_read_files) and (fn not in ever_write_files) and ret>0):
							init_read_files.append(fn)
				else:
					print("Should've matched (read_parse): " +line)
				continue
					
			if line.startswith("write("):
				mat = write_parse.match(line)
				if(mat):
					fd = mat.group(1)
					key = pid + "-" + fd
					if key in fd_fn_table:
						ret = int(mat.group(2))
						fn = fd_fn_table[key]
						if (fn not in ever_write_files) and ret>0:
							ever_write_files.append(fn)
				else:
					print("Should've matched (write_parse): " +line)
				continue
				
			if line.startswith("dup"):
				mat = dup_parse.match(line)
				if(mat):
					new_fd = int(mat.group(2))
					if(new_fd>=0):
						old_fd = int(mat.group(1))
						old_key = pid + "-" + str(old_fd)
						if old_key in fd_fn_table:
							fn = fd_fn_table[old_key]
							new_key = pid + "-" + str(new_fd)
							if new_key in fd_fn_table:
								del fd_fn_table[new_key]
							fd_fn_table[new_key] = fn
				else:
					print("Should've matched (dup_parse): " +line)
				continue
				
			#print("Unused logline: " + line)
	
	if remove_deleted_files:
		init_read_files = [fname for fname in init_read_files if os.path.isfile(fname)]
		ever_write_files = [fname for fname in ever_write_files if os.path.isfile(fname)]
	
	#Output the information
	with open(dep_file, "w") as dep_file:
		for fname in ever_write_files:
			fname_rel = os.path.relpath(fname)
			dep_file.write(fname_rel + ' ')
		
		dep_file.write(' : ')
			
		for fname in init_read_files:
			fname_rel = os.path.relpath(fname)
			dep_file.write(' ' + fname_rel)
		
		dep_file.write('\n')

if  __name__ =='__main__':
	main(sys.argv)
	
