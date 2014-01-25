#!/usr/bin/python
from commands import getstatusoutput
import sys, os

paths = (p for p in sys.argv[1:] if p.lower().endswith('.mp3'))
for path in paths:
	prefix = os.path.splitext(path)[0]
	cmd = 'afconvert -f WAVE -d LEI16@44100 "%s" "%s.wav"' % (path, prefix)
	print cmd
	status, output = getstatusoutput(cmd)
	if status:
		print "ERROR :( :("
		print output
