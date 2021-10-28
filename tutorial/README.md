Welcome to the labwork for the MPI Fault Tolerant hands-on.
===========================================================

This archive contains a set of example to be used to understand how
the ULFM resilient constructs can be used. The most up-to-date version
can be found either on http://fault-tolerance.org or in the GitHub
repository at https://github.com/ICLDisco/ulfm-testing.

More information on this package and how to use it is available from
[fault-tolerance.org/sc21](http://fault-tolerance.org/sc21).

This hands-on demonstrates how to use User Level Failure Mitigation
(ULFM) MPI to design typical application fault-tolerance patterns.
More examples, source code for the ULFM MPI implementation, and 
resources for designed fault tolerant HPC applications can
be accessed from [fault-tolerance.org](http://fault-tolerance.org).
The latest draft of the ULFM specification can be found
at [fault-tolerance.org/ulfm/ulfm-specification](http://fault-tolerance.org/ulfm/ulfm-specification/).


This package contains
---------------------

1. The tutorial hands-on examples.
2. Scripts to obtain and setup your shell for executing the examples in the
   pre-compiled Docker version of ULFM Open MPI v5.0.0rc2.

Using the Docker Image
----------------------

1. Install Docker
  * Docker can be seen as a "lightweight" virtual machine.
  [Docker documentation](https://docs.docker.com/engine/docker-overview/);
  [Docker cheat sheet](http://files.zeroturnaround.com/pdf/zt_docker_cheat_sheet.pdf).
  * Docker is available for a wide range of systems (MacOS, Windows, Linux).
  * You can install Docker quickly, either by downloading one of the official
  builds for [MacOS](https://download.docker.com/mac/stable/Docker.dmg) and
  [Windows](https://download.docker.com/win/stable/Docker%20for%20Windows%20Installer.exe),
  or by installing Docker from your Linux package manager (e.g.
  `yum install docker`, `apt-get docker-io`, etc.)
2. In a terminal, Run `docker run hello-world` to verify that the docker
installation works.
3. Load the docker script to map the `make`, `mpirun` and friends command to
execute from the Docker machine on the local directory.
  + In Linux, source the docker aliases in a terminal, `source dockervars.sh`;
  or `source dockervars.csh` (See SELinux note below.)
  + In Windows Powershell, `. dockervars.ps1`. In certain versions of Windows
  you need to enable script execution, which can be set temporarily for the
  duration of your Powershell session by issuing the command
  `$env:PSExecutionPolicyPreference="unrestricted"`
  + This script will also automate pulling the precompiled ULFM docker image
  (i.e., it issues `docker pull abouteiller/mpi-ft-ulfm`.)
4. In the tutorial examples directory. You can now type `make` to
compile the examples using the Docker provided "mpicc", and you can execute
the generated examples in the Docker machine using `mpirun -np 10 --with-ft ulfm example`
(In Windows, do **not** use `.\example`.)


__A note about Docker volume binding with SELinux__
The `dockervars.sh` script remapping uses Docker volumes to expose the source
code of the tutorial examples to the `mpicc` compiler inside the docker machine.
In SELinux enabled machines, exposing a host directory to the docker machine 
requires that this directory is properly labeled; [More information](https://www.projectatomic.io/blog/2015/06/using-volumes-with-docker-can-cause-problems-with-selinux/).
For simplicity, the remapping script drops the security label option within
the container; it will however still change user to match the current host 
user and drop other unecessary capabilities. If you want to still use SELinux
labeling, you should finely control which directories are labeled for docker usage
and explicitly label the examples directory with `chcon`; this is outside the
scope of this tutorial. Another, simpler, but more dangerous option is using
the automatic relabeling flag `:V`. While it can ensure proper labelling of the
exposed directories, when used improperly it may relabel unexpected directories
on the host: it is safe as long as you do not `cd` outside the example code
arborescence, but may relabel unexpected directories on the host when used 
in system directories, or at the root of a user's home; use at your own risk.

----------------------------------------------------------------------------

Alternative: Compiling ULFM Open MPI from source
------------------------------------------------

We recommend you use the Docker Image on your local laptop for the tutorial.

However, for performance and scalability testing on real high performance
systems, we strongly advice against using the Docker Image. Instead, you
may compile your own version of
[ULFM MPI](https://github.org/open-mpi/ompi) with support for your
prefered NIC and batch scheduler. Please refer to the complete instructions
on how to use ULFM, which is including in Open MPI version 5.0 onward, directly from its own
[README-ULFM](https://github.com/open-mpi/ompi/blob/master/README.FT.ULFM.md).

Expect the compilation to last between 15 to 20 minutes depending on your
setup.


----------------------------------------------------------------------------

When submitting questions and problems, be sure to include as much
extra information as possible. The best way to report bugs, send
comments, or ask questions is to sign up on the user's mailing list
ulfm@googlegroups.com.


----------------------------------------------------------------------------

Copyright (c) 2009-2021 The University of Tennessee and The University
                        of Tennessee Research Foundation.  All rights
                        reserved.

----------------------------------------------------------------------------
