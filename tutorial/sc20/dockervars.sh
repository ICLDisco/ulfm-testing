#/bin/sh
#
# Load some alias to compile and run with the docker mpicc/mpif90/mpirun
#

if [ $0 == $BASH_SOURCE ]; then
    echo "source $0 load # This file needs to be sourced, not executed."
    exit 2
fi

ulfm_image=abouteiller/mpi-ft-ulfm

case _$1 in
    _|_load)
        docker pull $ulfm_image
        function make {
            docker run -v $PWD:/sandbox:Z $ulfm_image make $@
        }
        function ompi_info {
            docker run $ulfm_image ompi_info $@
        }
        function mpirun {
            docker run -v $PWD:/sandbox:Z $ulfm_image mpirun --oversubscribe -mca btl tcp,self $@
        }
        function mpiexec {
            docker run -v $PWD:/sandbox:Z $ulfm_image mpiexec --oversubscribe -mca btl tcp,self $@
        }
        function mpiexec+gdb {
            docker run -v $PWD:/sandbox:Z --cap-add=SYS_PTRACE --security-opt seccomp=unconfined $ulfm_image mpiexec --oversubscribe -mca btl tcp,self $@
        }
        function mpicc {
            docker run -v $PWD:/sandbox:Z $ulfm_image mpicc $@
        }
        function mpif90 {
            docker run -v $PWD:/sandbox:Z $ulfm_image mpif90 $@
        }
        echo "#  Function alias set for 'make', 'mpirun', 'mpiexec', 'mpicc', 'mpif90'."
        echo "source $BASH_SOURCE unload # remove these aliases."
        echo "#    These commands now run from the ULFM Docker image."
        mpirun --version
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
        echo "source $BASH_SOURCE load # alias make, mpirun, etc. in the current shell"
        echo "source $BASH_SOURCE unload # remove aliases from the current shell"
esac


