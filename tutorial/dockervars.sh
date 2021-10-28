#/bin/sh
#
# Load some alias to compile and run with the docker mpicc/mpif90/mpirun
#

# No posix portable way to find if script is sourced;
# use some uglyness found on stackoverflow to handle bash, zsh, ksh...
sourced=0
if [ -n "$ZSH_EVAL_CONTEXT" ]; then
  case $ZSH_EVAL_CONTEXT in *:file) sourced=1;; esac
elif [ -n "$KSH_VERSION" ]; then
  [ "$(cd $(dirname -- $0) && pwd -P)/$(basename -- $0)" != "$(cd $(dirname -- ${.sh.file}) && pwd -P)/$(basename -- ${.sh.file})" ] && sourced=1
else # All other shells: examine $0 for known shell binary filenames
  # Detects `sh` and `dash`; add additional shell filenames as needed.
  case ${0##*/} in bash|sh|dash) sourced=1;; esac
fi
if [ 0 -eq $sourced ]; then
    echo "# This file needs to be sourced, not executed."
    echo "    source dockervars.sh load"
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
            docker run -v $PWD:/sandbox:Z $ulfm_image mpirun --map-by :oversubscribe --mca btl tcp,self $@
        }
        function mpiexec {
            docker run -v $PWD:/sandbox:Z $ulfm_image mpiexec --map-by :oversubscribe --mca btl tcp,self $@
        }
        function mpiexec+gdb {
            docker run -v $PWD:/sandbox:Z --cap-add=SYS_PTRACE --security-opt seccomp=unconfined $ulfm_image mpiexec --map-by :oversubscribe --mca btl tcp,self $@
        }
        function mpicc {
            docker run -v $PWD:/sandbox:Z $ulfm_image mpicc $@
        }
        function mpif90 {
            docker run -v $PWD:/sandbox:Z $ulfm_image mpif90 $@
        }
        echo "#  Function alias set for 'make', 'mpirun', 'mpiexec', 'mpicc', 'mpif90'."
        echo "source dockervars.sh unload # remove these aliases."
        echo "#    These commands now run from the ULFM Docker image."
        echo "#    Use \`mpiexec --with-ft ulfm\` to turn ON fault tolerance."
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
        echo "source dockervars.sh load # alias make, mpirun, etc. in the current shell"
        echo "source dockervars.sh unload # remove aliases from the current shell"
esac

