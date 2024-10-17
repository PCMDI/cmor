import os
import re
import sys
import time
import argparse

parser = argparse.ArgumentParser(
    description='Rebuild meta.yaml from conda-forge feedstock',
    formatter_class=argparse.ArgumentDefaultsHelpFormatter)

parser.add_argument("-p", "--package_name",
                    help="Package name to build")
parser.add_argument("-o", "--organization",
                    help="github organization name", default="PCMDI")
parser.add_argument("-r", "--repo_name",
                    help="repo name to build", default="cmor")
parser.add_argument("-b", "--branch", default='main', help="branch to build")
parser.add_argument("-v", "--version",
                    help="version are we building for")
parser.add_argument("-g", "--git_rev",
                    help="Latest commit")
parser.add_argument("-B", "--build", default="0",
                    help="build number, this should be 0 for nightly")
parser.add_argument("--local_repo", help="Path to local project repository")
parser.add_argument("--src_meta_yaml", help="Path to source meta.yaml")
parser.add_argument("--dst_meta_yaml", help="Path to destination meta.yaml")

args = parser.parse_args(sys.argv[1:])

print("Rebuilding meta.yaml with the following...")
for k, v in vars(args).items():
    print(f"{k}: {v}")

repo_url = f"https://github.com/{args.organization}/{args.repo_name}.git"

package_time = time.strftime("%Y.%m.%d.%H.%M", time.localtime())
version = f"{args.version}.{package_time}.{args.git_rev}"

with open(args.src_meta_yaml, "r") as src, \
        open(args.dst_meta_yaml, "w") as dst:
    start_copy = True
    lines = src.readlines()
    for line in lines:
        match_obj = re.match("package:", line)
        if match_obj:
            start_copy = False
            dst.write("package:\n")
            dst.write(f"  name: {args.package_name}\n")
            dst.write(f"  version: {version}\n\n")

            dst.write("source:\n")
            if args.local_repo is None:
                dst.write(f"  git_rev: {args.branch}\n")
                dst.write(f"  git_url: {repo_url}\n\n")
            else:
                dst.write(f"  path: {args.local_repo}\n\n")

            continue

        match_obj = re.match("build:", line)
        if match_obj:
            start_copy = True

        match_build_number = re.match(r"\s+number:", line)
        if match_build_number:
            dst.write(f"  number: {args.build}\n")
            continue
        if start_copy:
            dst.write(line)
        else:
            continue
