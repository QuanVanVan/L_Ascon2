#!/bin/bash
# Chay xen ke 7 lan moi phien ban, ghim vao CPU 2.
for i in 1 2 3 4 5 6 7; do
  for v in opt64 opt64_l_ascon2 ref ref_l_ascon2; do
    echo "### run $i $v"; taskset -c 2 ./getcycles_$v | sed -n '/cycles per byte/,/^$/p'
    taskset -c 2 ./perm_$v
  done
done
