doskey make='docker run -v %cd%:%cd%:Z -w %cd% bnicolae/veloc-tutorial make'
doskey mpirun='docker run -v %cd%:%cd%:Z -w %cd% bnicolae/veloc-tutorial mpirun --oversubscribe -mca btl tcp,self'
doskey mpiexec='docker run -v %cd%:%cd%:Z -w %cd% bnicolae/veloc-tutorial mpiexec --oversubscribe -mca btl tcp,self'
doskey mpicc='docker run -v %cd%:%cd%:Z -w %cd% bnicolae/veloc-tutorial mpicc'
doskey mpif90='docker run -v %cd%:%cd%:Z -w %cd% bnicolae/veloc-tutorial mpif90'
echo "#  Alias set for 'make', 'mpirun', 'mpiexec', 'mpicc', 'mpif90'."
echo "#    These commands now run from the ULFM+VeloC Docker image."

