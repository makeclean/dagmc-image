#!/bin/bash

rm -rf image_2_dagmc radiograph

g++ image_2_dagmc.cpp -I$HOME/opt/moab/include -L$HOME/opt/moab/lib -lMOAB -g -o image_2_dagmc

g++ radiograph.cpp -I$HOME/opt/moab/include -I$HOME/dev/dagmc-master/include -L$HOME/dev/dagmc-master/lib -ldagmc -L$HOME/opt/moab/lib -lMOAB -o radiograph
