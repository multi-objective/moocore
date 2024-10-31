#!/usr/bin/env python3
import os
import subprocess
import sys
import glob
import time
import tempfile
from joblib import Parallel, delayed


def runcmd(command):
    result = subprocess.run(command, shell=True)
    return result.returncode


def is_exe(fpath):
    fpath = os.path.expanduser(fpath)
    return (
        os.path.isfile(fpath)
        and os.access(fpath, os.X_OK)
        and os.path.getsize(fpath) > 0
    )


def run_test(test, program, dir_out):
    outfile = os.path.join(dir_out, test.replace(".test", ".out"))
    expfile = test.replace(".test", ".exp")
    compress_out = "| cat "  # So that redirection works
    diff = "diff"
    if not os.access(expfile, os.R_OK) and os.access(expfile + ".xz", os.R_OK):
        expfile = expfile + ".xz"
        outfile = outfile + ".xz"
        compress_out = " | xz "
        diff = "xzdiff"

    print("{:<60}".format("Running " + test + " :"), end=" ")
    command = f"PROGRAM={program} . ./{test} 2>&1 {compress_out}1> {outfile}"
    start_time = time.time()
    runcmd(command)
    elapsed_time = time.time() - start_time

    if runcmd(f"{diff} -iEBwq -- {expfile} {outfile} 1> /dev/null  2>&1") == 0:
        print(f"passed  {elapsed_time:6.2f}")
        os.remove(outfile)
        return True
    else:
        print(f"FAILED! {elapsed_time:6.2f}")
        print(f"{diff} -uiEBw -- {expfile} {outfile}")
        print(subprocess.getoutput(f"{diff} -uiEBw -- {expfile} {outfile}"))
        return False


def main():
    if len(sys.argv) < 2:
        print("usage:", sys.argv[0], "PROGRAM [TESTS]")
        print("\t for example:", sys.argv[0], "../bin/hv")
        sys.exit(1)

    program = os.path.expanduser(sys.argv[1])
    if not is_exe(program):
        print(f"error: '{program}' not found or not executable!")
        sys.exit(1)

    tests = sorted(glob.glob("*.test")) if len(sys.argv) == 2 else sys.argv[2:]
    for test in tests:
        if not test.endswith(".test"):
            print(test, "is not a test file")
            sys.exit(1)

    ntotal = len(tests)
    dir_out = tempfile.TemporaryDirectory()
    # print(dir_out)
    # os.makedirs(dir_out, exist_ok=True)
    ok = Parallel(n_jobs=-2)(
        delayed(run_test)(test, dir_out=dir_out.name, program=program) for test in tests
    )
    dir_out.cleanup()

    npassed = sum(ok)
    nfailed = ntotal - npassed
    print("\n === regression test summary ===\n")
    print(f"# of total tests : {ntotal:5d}")
    print(f"# of passed tests: {npassed:5d}")
    print(f"# of failed tests: {nfailed:5d}\n")
    exitcode = 1 if nfailed > 0 else 0
    sys.exit(exitcode)


if __name__ == "__main__":
    main()
