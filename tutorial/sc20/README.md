Welcome to the labwork for the MPI Fault Tolerant hands-on.
===========================================================

This package and information about it is available from [fault-tolerance.org/sc17](http://fault-tolerance.org/sc17).

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
   pre-compiled Docker version of ULFM Open MPI 2.1a1.

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
the generated examples in the Docker machine using `mpirun -np 10 example`
(In Windows, do **not** use `.\example`.)


__A note about Docker with SELinux__ The `dockervars.sh` script remapping
uses Docker volumes to expose the source code of the tutorial examples to the
`mpicc` compiler inside the docker machine. In SELinux enabled machines,
exposing a host directory to the docker machine requires that this directory
is properly labeled; [More information](https://www.projectatomic.io/blog/2015/06/using-volumes-with-docker-can-cause-problems-with-selinux/).
The remapping script employs the automatic relabeling flag `:V` to ensure
proper labelling of the exposed directories. If you want to finely control
which directories are labeled for docker usage, as an alternative, you may
suppress the `:V` flag from the script and explicitly label the examples
directory with `chcon`.


----------------------------------------------------------------------------

Alternative: Compiling ULFM Open MPI from source
------------------------------------------------

We recommend you use the Docker Image on your local laptop for the tutorial.

However, for performance and scalability testing on real high performance
systems, we strongly advice against using the Docker Image. Instead, you
may compile your own version of
[ULFM MPI](https://bitbucket.org/icldistcomp/ulfm2) with support for your
prefered NIC and batch scheduler. Please refer to the complete instructions
on how to install the ULFM Open MPI version 2.0rc directly from its own
README.

Expect the compilation to last between 15 to 20 minutes depending on your
setup.