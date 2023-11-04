#/bin/sh
#
# Load some alias to compile and run with the podman mpicc/mpif90/mpirun
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
    echo "    source podmanvars.sh load"
    exit 2
fi

ulfm_image=abouteiller/mpi-ft-ulfm

case _$1 in
    _|_load)
        podman pull $ulfm_image
        function make {
            podman run --userns=keep-id --user $(id -u):$(id -g)  --security-opt label=disable --cap-drop=all -v $PWD:/sandbox $ulfm_image make $@
        }
        function ompi_info {
            podman run --userns=keep-id --user $(id -u):$(id -g)  $ulfm_image ompi_info $@
        }
        function mpirun {
            podman run --userns=keep-id --user $(id -u):$(id -g)  --security-opt label=disable --cap-drop=all -v $PWD:/sandbox $ulfm_image mpirun --map-by :oversubscribe --mca btl tcp,self $@
        }
        function mpiexec {
            podman run --userns=keep-id --user $(id -u):$(id -g)  --security-opt label=disable --cap-drop=all -v $PWD:/sandbox $ulfm_image mpiexec --map-by :oversubscribe --mca btl tcp,self $@
        }
        function mpiexec+gdb {
            podman run --userns=keep-id --user $(id -u):$(id -g)  --security-opt label=disable --cap-drop=all -v $PWD:/sandbox --cap-add=SYS_PTRACE --security-opt seccomp=unconfined $ulfm_image mpiexec --map-by :oversubscribe --mca btl tcp,self $@
        }
        function mpicc {
            podman run --userns=keep-id --user $(id -u):$(id -g)  --security-opt label=disable --cap-drop=all -v $PWD:/sandbox $ulfm_image mpicc $@
        }
        function mpif90 {
            podman run --userns=keep-id --user $(id -u):$(id -g)  --security-opt label=disable --cap-drop=all -v $PWD:/sandbox $ulfm_image mpif90 $@
        }
        echo "#  Function alias set for 'make', 'mpirun', 'mpiexec', 'mpicc', 'mpif90'."
        echo "source podmanvars.sh unload # remove these aliases."
        echo "#    These commands now run from the ULFM podman image."
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
        echo "#  After this script is sourced in your local shell, these commands would run from the ULFM podman image."
        echo "source podmanvars.sh load # alias make, mpirun, etc. in the current shell"
        echo "source podmanvars.sh unload # remove aliases from the current shell"
esac

