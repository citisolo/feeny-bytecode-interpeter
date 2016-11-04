#!/bin/bash
echo "compiling {$1}"

./compiler -i $1 -o output/$2
