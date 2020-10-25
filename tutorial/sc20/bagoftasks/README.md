Bag of task example
===================

Try to upgrade this bag of task framework to support run-through error with MPI.

The solution involves using the ULFM and MPI constructs demonstrated during the tutorial so far.
* `MPI_COMM_CREATE_ERRHANDLER()`
* `MPI_COMM_SET_ERRHANDLER()`
* `MPIX_COMM_FAILURE_ACK()`
* `MPIX_COMM_FAILURE_GET_ACKED()`
* `MPI_GROUP_TRANSLATE_RANKS()`

If you want to draw inspiration, you can look at example `../02.err_handler.c`.

A full solution is provided in the `answer` directory.
Note that the `answer` code demonstrates both run-through (i.e., blank mode) and
shrink-on-error modes.

