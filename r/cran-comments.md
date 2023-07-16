Dear CRAN maintainers,

I would like to submit a new version of the 'eaf' package to CRAN.

This fixes a -Wuse-after-free warning reported by Prof Brian Ripley.

Regarding this NOTE:

* checking compiled code ... NOTE
File ‘eaf/libs/eaf.so’:
  Found ‘___stderrp’, possibly from ‘stderr’ (C)
    Objects: ‘eaf/io.o’, ‘mo-tools/hv.o’
  Found ‘___stdoutp’, possibly from ‘stdout’ (C)
    Object: ‘eaf/io.o’
Flavors: r-release-macos-x86_64, r-oldrel-macos-x86_64

The same user-code is compiled for all operating systems. There is no conditional code depending on OS. This NOTE only happens for MacOS. My conclusion is that the C-library used by Clang in MacOS seems to pull stderr/stdout into user code even if the user does not use stderr/stdout. I do not know what I can do to avoid this. There are no uses of stderr/stdout in my code when compiled as an R package.

The other NOTE:

Check: installed package size
Result: NOTE
     installed size is 6.4Mb
     sub-directories of 1Mb or more:
     bin 3.4Mb
     extdata 1.7Mb
Flavors: r-devel-windows-x86_64-new-UL, r-release-windows-ix86+x86_64, r-oldrel-windows-ix86+x86_64

The package installs some optional command-line tools. For some reason unknown to me, these executables are larger in Windows than in other OSs. They are built from the same source code.

GNU make is a SystemRequirements because of the Makefiles used to build the various pieces of code under src/.

Thanks in advance,

    Manuel López-Ibáñez.
