#/bin/csh
#
# Load some alias to compile and run with the docker mpicc/mpif90/mpirun
#

set name = ($_)
if($#name < 2) then
    echo "source $0 load # this file needs to be sourced, not executed."
    exit 2
else
    set name = $name[2]
endif

switch(_$1)
    case _:
    case _load:
        docker pull abouteiller/mpi-ft-ulfm
        alias make 'docker run -v ${PWD}:/sandbox:Z abouteiller/mpi-ft-ulfm make'
        alias ompi_info 'docker run abouteiller/mpi-ft-ulfm ompi_info'
        alias mpirun 'docker run -v ${PWD}:/sandbox:Z abouteiller/mpi-ft-ulfm mpirun --oversubscribe -mca btl tcp,self'
        alias mpiexec 'docker run -v ${PWD}:/sandbox:Z abouteiller/mpi-ft-ulfm mpiexec --oversubscribe -mca btl tcp,self'
        alias mpicc 'docker run -v ${PWD}:/sandbox:Z abouteiller/mpi-ft-ulfm mpicc'
        alias mpif90 'docker run -v ${PWD}:/sandbox:Z abouteiller/mpi-ft-ulfm mpif90'
        echo "#  Function alias set for 'make', 'mpirun', 'mpiexec', 'mpicc', 'mpif90'."
        echo "source $name unload # remove these aliases."
        echo "#    These commands now run from the ULFM Docker image."
        mpirun --version
        breaksw
    case _unload:
        unalias make
        unalias ompi_info
        unalias mpiexec
        unalias mpirun
        unalias mpicc
        unalias mpif90
        echo "#  Function alias unset for 'make', 'mpirun', 'mpiexec', 'mpicc', 'mpif90'."
        breaksw
    default:
        echo "#  This script is designed to load aliases for 'make', 'mpirun', 'mpiexec', 'mpicc', 'mpif90'."
        echo "#  After this script is sourced in your local shell, the commands provided by the ULFM Docker image operate on the local files."
        echo "source $name load # alias make and mpirun in the current shell"
        echo "source $name unload # remove aliases from the current shell"
endsw


