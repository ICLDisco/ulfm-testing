# Windows PowerShell
# Load some function to compile and run with the docker mpicc/mpif90/mpirun
#


Switch -Regex ($args[0]) {
    "^$|^load$" {
        docker pull abouteiller/mpi-ft-ulfm
        function make { docker run -v ${PWD}:/sandbox:Z abouteiller/mpi-ft-ulfm make $args }
        function ompi_info { docker run -v ${PWD}:/sandbox:Z abouteiller/mpi-ft-ulfm ompi_info $args }
        function mpirun { docker run -v ${PWD}:/sandbox:Z abouteiller/mpi-ft-ulfm mpirun --oversubscribe -mca btl tcp,self $args }
        function mpiexec { docker run -v ${PWD}:/sandbox:Z abouteiller/mpi-ft-ulfm mpiexec --oversubscribe -mca btl tcp,self $args }
        function mpicc { docker run -v ${PWD}:/sandbox:Z abouteiller/mpi-ft-ulfm mpicc $args }
        function mpif90 { docker run -v ${PWD}:/sandbox:Z abouteiller/mpi-ft-ulfm mpif90 $args }
        echo "#  alias functions set for 'make', 'mpirun', 'mpiexec', 'mpicc', 'mpif90'."
        echo (". " + $MyInvocation.MyCommand.Name + " unload; remove these alias functions.")
        echo "#    These commands now run from the ULFM Docker image."
        mpirun --version
    }
    "^unload$" {
        Remove-Item -ErrorAction SilentlyContinue -path Function:make
        Remove-Item -ErrorAction SilentlyContinue -path Function:ompi_info
        Remove-Item -ErrorAction SilentlyContinue -path Function:mpirun
        Remove-Item -ErrorAction SilentlyContinue -path Function:mpiexec
        Remove-Item -ErrorAction SilentlyContinue -path Function:mpicc
        Remove-Item -ErrorAction SilentlyContinue -path Function:mpif90
        echo "function unset for 'make', 'mpirun', 'mpiexec', 'mpicc', 'mpif90'."
    }
    default {
        echo (". " + $MyInvocation.MyCommand.Name + " load: function alias 'make', 'mpirun', 'mpiexec', 'mpicc', and 'mpif90' in the current shell")
        echo (". " + $MyInvocation.MyCommand.Name + " unload: unset function aliases from the current shell")
    }
}
