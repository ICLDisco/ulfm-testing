#/bin/sh
#
# Load some alias to compile and run with the docker mpicc/mpif90/mpirun
#

DOCKER=bnicolae/veloc-tutorial

case _$1 in
    _|_load)
        alias make='docker run -v $PWD:$PWD:Z -w $PWD '$DOCKER' make'
        alias mpirun='docker run -v $(pwd):$(pwd):Z -w $(pwd) '$DOCKER' mpirun --oversubscribe -mca btl tcp,self'
        alias mpiexec='docker run -v $(pwd):$(pwd):Z -w $(pwd) '$DOCKER' mpiexec --oversubscribe -mca btl tcp,self'
        alias mpicc='docker run -v $(pwd):$(pwd):Z -w $(pwd) '$DOCKER' mpicc'
        alias mpif90='docker run -v $(pwd):$(pwd):Z -w $(pwd) '$DOCKER' mpif90'
        echo "#  Alias set for 'make', 'mpirun', 'mpiexec', 'mpicc', 'mpif90'."
        echo "#    These commands now run from the ULFM+VeloC Docker image."
        echo "#    Run . $0 unload to remove these aliases."
        ;;
    _unload)
        unalias make
        unalias mpirun
        unalias mpiexec
        unalias mpicc
        unalias mpif90
        echo "Alias unset for 'make', 'mpirun', 'mpiexec', 'mpicc', 'mpif90'."
        ;;
    *)
        echo ". $0 load: alias make and mpirun in the current shell"
        echo ". $0 unload: remove aliases from the current shell"
esac


