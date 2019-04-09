#!/usr/bin/env python
from __future__ import print_function
import argparse
import sys
import glob
import time
import os

if "VERSION" in os.environ.keys():
    last_stable=os.environ['VERSION']
else:
    last_stable = "8.0"

l = time.localtime()
today = "%s.%.2i.%.2i.%.2i.%.2i.%.2i.{{ GIT_DESCRIBE_HASH }}" % (last_stable, l.tm_year, l.tm_mon, l.tm_mday, l.tm_hour, l.tm_min)

parser = argparse.ArgumentParser(
    description='Cleanup your anaconda server',
    formatter_class=argparse.ArgumentDefaultsHelpFormatter)

parser.add_argument("-b", "--branch", default="master",
                    help="branch to use on uvcdat repo")
parser.add_argument(
    "-B",
    "--build",
    default="0",
    help="name of this build")

parser.add_argument("-v", "--version", default=today,
                    help="which version are we building")

parser.add_argument("-l", "--last_stable", default=last_stable,
                    help="which version are we building")

parser.add_argument("-f", "--features", nargs="*",
                    help="features to be enabled", default=[])


args = parser.parse_args(sys.argv[1:])

today2 = "%s.%.2i.%.2i.%.2i.%.2i.%.2i.{{ GIT_DESCRIBE_HASH }}" % (args.last_stable, l.tm_year, l.tm_mon, l.tm_mday, l.tm_hour, l.tm_min)
featured_packages = {}
files = glob.glob("*/meta.yaml.in")
for fnm in files:
    print(fnm, args.version,today)
    with open(fnm) as f:
        s = f.read()
    s = s.replace("@UVCDAT_BRANCH@", args.branch)
    s = s.replace("@BUILD_NUMBER@", args.build)
    if args.version != today:
        s = s.replace("@VERSION@", args.version)
    else:
        s = s.replace("@VERSION@", today2)

    out = []
    for l in s.split("\n"):
        if any([l.find("{{{ %s }}}" % f)>-1 for f in args.features]):
            for f in args.features:
                l = l.replace("{{{ %s }}}" % f,"")
            out.append(l)
        elif l.find("{{{")==-1:  # it's not a unused feature line
            out.append(l)
    with open(fnm[:-3],"w") as f:
        f.write("\n".join(out))
