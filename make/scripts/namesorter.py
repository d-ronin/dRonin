#!/usr/bin/env python

import subprocess
import json

outp = subprocess.check_output(["git", "log", "--pretty=format:%at:%aN", "--no-merges"])

lines = str.splitlines(outp)

people = {}

for i in range(0, len(lines)):
    (tm,nm) = lines[i].split(':',2)

    tm=int(tm)

    if i == 0:
        lastTm = tm

    yearsOld = (lastTm-tm) / 31536000.0

    people[nm] = people.get(nm, 0) - (pow(2, -yearsOld/1.5))

peopleSorted = sorted(people, key=people.get)

print "const char credits[] = ", json.dumps(', '.join(peopleSorted)), ";\n"
