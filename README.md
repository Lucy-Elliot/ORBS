VERSION 1

Depenencies: Raylib 5.0+, C++17, and Blender 4.0+.


Controls are available to view in the Help Modal once ORBS is up and running <3


Blender 4.0 is needed as this code uses geometry nodes and is not compatable with earlier versions.

to run on a HPC upload the python code, load blender module or install blender 4.0+, change the file path in render_job.py and run:
start blender -b -P render_job.py


Known issues:
 - If you try to use blender without a camera_path.py file but with an electron box selected, it will not work/render, I have not made an error yet.
 - Help modal doesnt scale correctly when changing text size.
