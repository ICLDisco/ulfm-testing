Welcome to the labwork for the MPI Fault Tolerant hands-on.
===========================================================

This package and information about it is available from [fault-tolerance.org/sc17](http://fault-tolerance.org/sc17)

This package contains
---------------------

1. Instructions to obtain a Docker Image of the pre-compiled version of
   ULFM Open MPI 2.0rc.
2. The tutorial hands-on example.
3. More examples, tests, information, as well as the latest draft of the
   ULFM specification can be found on [fault-tolerance.org/ulfm/ulfm-specification](http://fault-tolerance.org/ulfm/ulfm-specification/)

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
3. Load the pre-compiled ULFM Docker machine into your Docker installation
`docker pull abouteiller/mpi-ft-ulfm`.
4. Source the docker aliases in a terminal, this will redirect the "make"
and "mpirun" command in the local shell to execute in the Docker machine.
`. dockervars.sh`.
5. Go to the tutorial examples directory. You can now type `make` to
compile the examples using the Docker provided "mpicc", and you can execute
the generated examples in the Docker machine using `mpirun -np 10 example`.






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
