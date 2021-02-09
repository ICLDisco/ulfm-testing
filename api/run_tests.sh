#!/bin/bash

# create output file
exec 1> >(tee $(date +%y-%m-%d.%R).log) 2>&1

# default values for test setup
prefix=${ULFM_PREFIX+$ULFM_PREFIX}
np=4
time=2m
#args="--mca btl_tcp_if_include ib1 --mca btl tcp,self" # for example, use TCPoIB on ib1 iface

while getopts "p:n:a:t:" OPTION; do
    case $OPTION in
    p) prefix=$OPTARG ;;
    n) np=$OPTARG ;;
    a) args=$OPTARG ;;
    t) time=$OPTARG ;;
    *) cat <<'EOF'
Invalid option provided

-p: prefix (path to root dir of the Open MPI installation)
-n: np (number of procs)
-a: args (extra arguments to pass to mpiexec)
-t: timeout (e.g., 10s, 6m, 2h)
EOF
    ;;
    esac
done
mpiexec="${prefix+$prefix/bin/}mpiexec $args"


function fail_test {
    failed="$failed $1"
}

set -o pipefail
date

echo "######################################################################"
echo "### TESTING 1: -initial-errhandler"
cmd="$mpiexec -np $np --initial-errhandler mpi_errors_return ./init_errh_info"
echo $cmd
echo "##yy####################################################################"
eval timeout $time "$cmd" | awk '
BEGIN{m=0} {print} /mpi_errors_return/{m++} END{if(m != '$np') {exit 1}}'\
    && echo -e "\n+++ TEST SUCCESS +++\n" || { echo -e "\n!!! TEST FAILED !!!\n" && fail_test 1; }
echo "######################################################################"
echo

echo "######################################################################"
echo "### TESTING 2: ULFM bindings"
cmd="$mpiexec -np $np --with-ft mpi  ./bindings"
echo $cmd
echo "######################################################################"
eval timeout $time "$cmd" | awk '
BEGIN{m=0} {print} /NOT COMPLIANT/{m++} END{if(0 != m) {exit 1}}'\
    && echo -e "\n+++ TEST SUCCESS +++\n" || { echo -e "\n!!! TEST FAILED !!!\n" && fail_test 2; }
echo "######################################################################"
echo

echo "######################################################################"
echo "### TESTING 3: --with-ft no"
cmd="$mpiexec -np $np --with-ft no ./bindings"
echo $cmd
echo "######################################################################"
eval timeout $time "$cmd" | awk '
BEGIN{m=0} {print} /CAN tolerate/{m++} END{if(0 != m) {exit 1}}'\
    && echo -e "\n+++ TEST SUCCESS +++\n" || { echo -e "\n!!! TEST FAILED !!!\n" && fail_test 3; }
echo "######################################################################"
echo

echo "######################################################################"
echo "### TESTING 4: ULFM error returns after failure"
cmd="$mpiexec -np $np --with-ft mpi ./err_returns"
echo $cmd
echo "######################################################################"
eval timeout $time "$cmd" | awk '
BEGIN{m=0} {print} /TEST PASSED/{m++} END{if(0 == m) {exit 1}}'\
    && echo -e "\n+++ TEST SUCCESS +++\n" || { echo -e "\n!!! TEST FAILED !!!\n" && fail_test 4; }
echo "######################################################################"
echo

echo "######################################################################"
echo "### TESTING 5: ULFM error handler after failure, 45s sleep"
cmd="$mpiexec -np $np --with-ft mpi ./err_handler"
echo $cmd
echo "######################################################################"
eval timeout $time "$cmd" | awk '
BEGIN{m=0} {print} /TEST PASSED/{m++} END{if(0 == m) {exit 1}}'\
    && echo -e "\n+++ TEST SUCCESS +++\n" || { echo -e "\n!!! TEST FAILED !!!\n" && fail_test 5; }
echo "######################################################################"
echo

echo "######################################################################"
echo "### TESTING 6: ULFM error during ANY_SOURCE after failure"
cmd="$mpiexec -np $np --with-ft mpi ./err_any"
echo $cmd
eval timeout $time "$cmd" | awk '
BEGIN{m=0} {print} /TEST PASSED/{m++} END{if(0 == m) {exit 1}}'\
    && echo -e "\n+++ TEST SUCCESS +++\n" || { echo -e "\n!!! TEST FAILED !!!\n" && fail_test 6; }
echo "######################################################################"
echo

echo "######################################################################"
echo "### TESTING 7: ULFM error insulation from failure in another comm"
cmd="$mpiexec -np $np --with-ft mpi ./err_insulation"
echo $cmd
eval timeout $time "$cmd" | awk '
BEGIN{m=0} {print} /TEST FAILED/{m++} END{if(0 < m) {exit 1}}'\
    && echo -e "\n+++ TEST SUCCESS +++\n" || { echo -e "\n!!! TEST FAILED !!!\n" && fail_test 7; }
echo "######################################################################"
echo

echo "######################################################################"
echo "### TESTING 8: ULFM number of errors reported from getack is compliant"
cmd="$mpiexec -np $np --with-ft mpi ./getack"
echo $cmd
eval timeout $time "$cmd" | awk '
BEGIN{m=0} {print} /TEST FAILED/{m++} END{if(0 < m) {exit 1}}'\
    && echo -e "\n+++ TEST SUCCESS +++\n" || { echo -e "\n!!! TEST FAILED !!!\n" && fail_test 8; }
echo "######################################################################"
echo

echo "######################################################################"
echo "### TESTING 9: ULFM revoke is compliant"
cmd="$mpiexec -np $np --with-ft mpi ./revoke"
echo $cmd
eval timeout $time "$cmd" \
    && echo -e "\n+++ TEST SUCCESS +++\n" || { echo -e "\n!!! TEST FAILED !!!\n" && fail_test 9; }
echo "######################################################################"
echo

echo "######################################################################"
echo "### TESTING 10: ULFM shrink after revoke is compliant"
cmd="$mpiexec -np $np --with-ft mpi ./revshrink"
echo $cmd
eval timeout $time "$cmd" | awk '
BEGIN{m=0} {print} /COMPLIANT @ repeat 99/{m++} END{if(0 == m) {exit 1}}'\
    && echo -e "\n+++ TEST SUCCESS +++\n" || { echo -e "\n!!! TEST FAILED !!!\n" && fail_test 10; }
echo "######################################################################"
echo

echo "######################################################################"
echo "### TESTING 11: ULFM shrink after failure is compliant"
cmd="$mpiexec -np $np --with-ft mpi ./revshrinkkill"
echo $cmd
eval timeout $time "$cmd" | awk '
BEGIN{k=0; f=0} {print} /Finalizing/{f++} /Killing Self/{k++} END{if(1 != f || '$((np-1))' != k) {exit 1}}'\
    && echo -e "\n+++ TEST SUCCESS +++\n" || { echo -e "\n!!! TEST FAILED !!!\n" && fail_test 11; }
echo "######################################################################"
echo

echo "######################################################################"
echo "### TESTING 12: ULFM shrink-spawn sequence can recover failures"
cmd="$mpiexec -np $np --with-ft mpi ./buddycr"
echo $cmd
eval timeout $time "$cmd" | awk '
BEGIN{m=0} {print} /starting bcast 5/{m++} END{if(0 == m) {exit 1}}'\
    && echo -e "\n+++ TEST SUCCESS +++\n" || { echo -e "\n!!! TEST FAILED !!!\n" && fail_test 12; }
echo "######################################################################"
echo

for rc in $failed; do
#loop will run only once, print all failed tests, and exit with the number of the first failed test
    echo
    echo "######################################################################"
    echo "!!! THE FOLLOWING TESTS FAILED: $failed"
    echo "######################################################################"
    echo
    exit $rc
done


