standalone.exe db -cmds=wv -pipeline -idxType=2 -bits=16  -inMem -noDocs -pennysort 40000000
standalone.exe db -cmds=wv -pipeline -idxType=1 -bits=16  -inMem -noDocs -pennysort 40000000
standalone.exe db -cmds=wv -pipeline -idxType=0 -bits=16  -inMem -noDocs -pennysort 40000000
