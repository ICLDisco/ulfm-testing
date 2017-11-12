doskey make='docker run -v %cd%:%cd%:Z -w %cd% abouteiller/mpi-ft-ulfm make'
doskey mpirun='docker run -v %cd%:%cd%:Z -w %cd% abouteiller/mpi-ft-ulfm mpirun --oversubscribe -mca btl tcp,self'
doskey mpiexec='docker run -v %cd%:%cd%:Z -w %cd% abouteiller/mpi-ft-ulfm mpiexec --oversubscribe -mca btl tcp,self'
doskey mpicc='docker run -v %cd%:%cd%:Z -w %cd% abouteiller/mpi-ft-ulfm mpicc'
doskey mpif90='docker run -v %cd%:%cd%:Z -w %cd% abouteiller/mpi-ft-ulfm mpif90'
echo "#  Alias set for 'make', 'mpirun', 'mpiexec', 'mpicc', 'mpif90'."
echo "#    These commands now run from the ULFM Docker image."

