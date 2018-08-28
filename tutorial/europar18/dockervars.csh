#/bin/csh
#
# Load some alias to compile and run with the docker mpicc/mpif90/mpirun
#
DOCKER=bnicolae/veloc-tutorial

switch($1)

    case load:
        alias make 'docker run -v $PWD:$PWD:Z -w $PWD '$DOCKER' make'
        alias mpirun 'docker run -v $(pwd):$(pwd):Z -w $(pwd) '$DOCKER' mpirun --oversubscribe -mca btl tcp,self'
        alias mpiexec 'docker run -v $(pwd):$(pwd):Z -w $(pwd) '$DOCKER' mpiexec --oversubscribe -mca btl tcp,self'
        alias mpicc 'docker run -v $(pwd):$(pwd):Z -w $(pwd) '$DOCKER' mpicc'
        alias mpif90 'docker run -v $(pwd):$(pwd):Z -w $(pwd) '$DOCKER' mpif90'
        breaksw
    case unload:
        unalias make
        unalias mpirun
        breaksw
    default:
        echo ". $0 load: alias make and mpirun in the current shell"
        echo ". $0 unload: remove aliases from the current shell"
endsw


