# README #

ULFM testings

### What is this repository for? ###

This repository contains test to verify an ULFM Fault Tolerant MPI implementation conformity to the ULFM specification, measure performance in failure cases and the cost of recovery operations, examples, and torture tests.

### How do I get set up? ###

* Install and compile an ULFM MPI implementation (from http://fault-tolerance.org, to use the Open MPI ULFM derivative, as an example)
* Set the variable ULFM_PREFIX in your shell
* Go in one of the subdirectories, make
* make run shows a typical mpirun command line to execute with fault tolerance enabled with the Open MPI ULFM version.

### Who do I talk to? ###

* bouteill@icl.utk.edu, bosilca@icl.utk.edu, herault@icl.utk.edu
* Join the mailing list: ulfm@googlegroups.com
* More information at http://fault-tolerance.org
* Original url for this repository https://bitbucket.org/icldistcomp/ulfm-testing
