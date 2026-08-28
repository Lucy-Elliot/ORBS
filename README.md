VERSION 1 (pre-alpha)
original python code by Matthew Allen, modified to add a camera and some aesthetic changes.



Depenencies: Raylib 5.0+, C++17, and Blender 4.0+.


Controls are available to view in the Help Modal once ORBS is up and running <3


Blender 4.0 is needed as this code uses geometry nodes and is not compatable with earlier versions.

to run on a HPC upload the python code, load blender module or install blender 4.0+, change the file path in render_job.py and run:
start blender -b -P render_job.py


install with:

git clone https://github.com/Lucy-Elliot/ORBS

cd ORBS

cmake -S . -B build

cmake --build build

