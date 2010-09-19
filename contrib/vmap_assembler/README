Linux:

1. Building

	cd to contrib/vmap_assembler/ and execute:

	$ cmake .
	$ make

	You should now have an executable file vmap_assembler

2. Assembling vmaps

	Use the created executable to create the vmap files for MaNGOS.
	The executable takes two arguments:

	vmap_assembler <input_dir> <output_dir>

	Example:
	$ ./vmap_assembler Buildings vmaps

	<output_dir> has to exist already and shall be empty.
	The resulting files in <output_dir> are expected to be found in ${DataDir}/vmaps
	by mangos-worldd (DataDir is set in mangosd.conf).

###########################
Windows:

1. Building

	Build the solution in contrib\vmap_assembler\
	Resulting binaries will be in contrib\vmap_assembler\bin\$(PlatformName)_$(ConfigurationName)\

2. Assembling vmaps

	Use the created executable (from command prompt) to create the vmap files for MaNGOS.
	The executable takes two arguments:

	vmap_assembler.exe <input_dir> <output_dir>

	Example:
	C:\my_data_dir\> vmap_assembler.exe Buildings vmaps

	<output_dir> has to exist already and shall be empty.
	The resulting files in <output_dir> are expected to be found in ${DataDir}\vmaps
	by mangos-worldd (DataDir is set in mangosd.conf).
