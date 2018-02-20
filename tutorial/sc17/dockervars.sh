#/bin/sh
#
# Load some alias to compile and run with the docker mpicc/mpif90/mpirun
#

case _$1 in
    _|_load)
        function make {
            docker run -v $PWD:/wd:Z -w /wd abouteiller/mpi-ft-ulfm make $@
        }
        function ompi_info {
            docker run abouteiller/mpi-ft-ulfm ompi_info $@
        }
        function mpirun {
            docker run -v $PWD:/wd:Z -w /wd abouteiller/mpi-ft-ulfm mpirun --oversubscribe -mca btl tcp,self $@
        }
        function mpiexec {
            docker run -v $PWD:/wd:Z -w /wd abouteiller/mpi-ft-ulfm mpiexec --oversubscribe -mca btl tcp,self $@
        }
        function mpicc {
            docker run -v $PWD:/wd:Z -w /wd abouteiller/mpi-ft-ulfm mpicc $@
        }
        function mpif90 {
            docker run -v $PWD:/wd:Z -w /wd abouteiller/mpi-ft-ulfm mpif90 $@
        }
        echo "#  Function alias set for 'make', 'mpirun', 'mpiexec', 'mpicc', 'mpif90'."
        echo "#    These commands now run from the ULFM Docker image."
        mpirun --version
        echo "#    Run . $0 unload to remove these aliases."
        ;;
    _unload)
        unset -f make
        unset -f ompi_info
        unset -f mpirun
        unset -f mpiexec
        unset -f mpicc
        unset -f mpif90
        echo "#  Function alias unset for 'make', 'mpirun', 'mpiexec', 'mpicc', 'mpif90'."
        ;;
    *)
        echo "#  This script is designed to load aliases for for 'make', 'mpirun', 'mpiexec', 'mpicc', 'mpif90'."
        echo "#  After this script is sourced in your local shell, these commands would run from the ULFM Docker image."
        echo ". $0 load: alias make and mpirun in the current shell"
        echo ". $0 unload: remove aliases from the current shell"
esac


