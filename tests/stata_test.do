clear all
mac drop _all
log close _all
log using stata_internal_log.smcl, replace

*Do some file work
sysuse auto
save auto_loc.dta, replace
use auto_loc, clear

*Write out a graph
twoway (scatter price mpg)
graph save Graph "Graph.gph", replace
graph export "Graph.eps", as(eps) preview(on) replace

*Output tables
reg price mpg
est save stata.est, replace
est use stata.est

**Low-level file access
*In Stata
file open fhandle using "stata_test.txt", write text replace
file write fhandle "Blah"
file close fhandle

file open fhandle using "stata_test.txt", write text append
file write fhandle "Blah2"
file close fhandle

file open fhandle using "stata_test.txt", read text
file read fhandle blankmacro
file close fhandle

*In Mata
cap erase "stata_mata.dat"
mata:
fhand = fopen("stata_mata.dat", "w")
fput(fhand, "yes")
fclose(fhand)

fhand = fopen("stata_mata.dat", "a")
fput(fhand, "no")
fclose(fhand)

fhand = fopen("stata_mata.dat", "r")
x = fget(fhand)
fclose(fhand)

end

log close _all
