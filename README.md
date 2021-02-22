# Dangling Pointers

Checks for dangling pointers

## Usage

In src/, build with

    $ mkdir obj/
    $ make PIN_ROOT=</path/to/Pin> obj/dangling.so

Run with

    $ </path/to/Pin> -t obj/dangling.so -- </path/to/executable> <executable_args>

Print additional information including backtraces with -v option:

    $ </path/to/Pin> -t obj/dangling.so -v 1 -- </path/to/executable> <executable_args>

All output is recorded in a dangling.out file.
