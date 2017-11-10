#/bin/csh
#
# Load some alias to compile and run with the docker mpicc/mpif90/mpirun
#

switch($1)

    case load:
        alias make 'docker run -v $PWD:$PWD -w $PWD abouteiller/mpi-ft-ulfm make'
        alias mpirun 'docker run -v $(pwd):$(pwd) -w $(pwd) abouteiller/mpi-ft-ulfm mpirun --oversubscribe -mca btl tcp,self'
        breaksw
    case unload:
        unalias make
        unalias mpirun
        breaksw
    default:
        echo ". $0 load: alias make and mpirun in the current shell"
        echo ". $0 unload: remove aliases from the current shell"
endsw


