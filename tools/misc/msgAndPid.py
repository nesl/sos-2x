#!/usr/bin/python

import os
import sys

if __name__ == '__main__':

    file = sys.argv[1]
    msg = os.popen("objdump -G " + file + " | grep MSG")
    pid = os.popen("objdump -G " + file + " | grep PID")

    msgMapping = {}
    pidMapping = {}

    for line in msg:
        pairs = line.split()[6].split(';')[1][1:].split(',')
        for pair in pairs:
            if "MSG" in pair:
                str, num = pair.split(':')
                msgMapping[str] = num

    for line in pid:
        pairs = line.split()[6].split(';')[1][1:].split(',')
        for pair in pairs:
            if "PID" in pair:
                str, num = pair.split(':')
                pidMapping[str] = num


    print "\n### MSG IDs ###"
    for pair in [(k,msgMapping[k]) for k in sorted(msgMapping.keys())]:
        print "%3s (0x%2x)\t%s"%(pair[1], int(pair[1]), pair[0])

    print "\n### PID IDs ###"
    for pair in [(k,pidMapping[k]) for k in sorted(pidMapping.keys())]:
        print "%3s (0x%2x)\t%s"%(pair[1], int(pair[1]), pair[0])
