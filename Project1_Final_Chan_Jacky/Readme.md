Jacky Chan PROJECT 1 HEIGHTMAPS
SUBMISSION CONTENTS

1. Project_1 folder containing:
	1. Headers
		camera.hpp
		heightmap.hpp
		Project1.hpp
		shader.hpp
	2. Media (contains textures and images used in the project)
	3. Shaders
	4. Source code
	5. A Project_1.exe to run the project
	6. MP4 showcasing requirements of project
	7. This Readme file.


HOW TO RUN THE PROJECT:
1. Extract and unzip the folders
2. Go to .../Project1_Final_Chan_Jacky/Project_1/
3. Run the Project_1.exe

FILENAMES
Project1.cpp
	Edited lines:
		Line 24- Initialized translation/rotation/scale vectors.
		Line 191 - Loading Skybox Textures
		Line 266 - Setting angles to rotate
		Line 274 - Rotation/scaling/translating box models to render
		Lines 319-394 - Making walls for skybox, setting textures, transforming them to fit appropriately
		Lines 435-485
			Lines 435-446 - Inputs for rotation
			Lines 447-458 - Inputs for scale
			Lines 460-471 - Inputs for translation
			Line 474 - Input for transformation reset

heightmap.ccp
	Edited Lines
		Line 72-85 - Loaded heightmap texture to draw triangles for heightmap
		Line 115-134 - Uncommented function and put in respective position and texture coordinate data
		Line 138 - Create_heightmap() function that does a double for loop to calculate every single vertex
		Line 153 - create_indicies() function creates indicies based on width and height (does this twice since only once creates triangles
		Line 177 - setup_heightmap() function. Basically just reuses code found in Project1.cpp to bind relevent vertex buffers and data

		
				
		
