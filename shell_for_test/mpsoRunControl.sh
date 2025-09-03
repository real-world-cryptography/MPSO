#!/bin/bash

echo "start for mpsi"

./run_mpsi.sh > mpsiRes.txt

echo -e "end for mpsi\n\n\n"


echo "start for mpsic"

./run_mpsic.sh > mpsicRes.txt

echo -e "end for mpsic\n\n\n"


echo "start for mpsics"

./run_mpsics.sh > mpsicsRes.txt

echo -e "end for mpsics\n\n\n"


echo "start for mpsu"

./run_mpsu.sh > mpsuRes.txt

echo -e "end for mpsu\n\n\n"
