Ayon OpenUsd Resolver (uses Ayon cpp Api)

requiered:
  C++ Compiler \n
  Cmake \n
  GitHub public key setup ( this is because the submodules are linked via git@ ) \n
  Houdini Install \n
Tested Platforms: \n
	Alma Linux 9 \n
	Windows 10 \n


Prebuild / Self Compiled

#Self Compiled:
Downloade the repo and its submodule: 
- git clone --recurse-submodules git@github.com:ynput/ayon-usd-resolver.git
- git submodule update --init --recursive


windows 
- set the houdini install directory inside off the build.bat file 
- run build.bat
no your resolver is compiled
- the dist foulder will now contain the ayonUsdResolver foulder. ( this is the compiled resolver ) 
- there is a launchHouWithResolver.bat point the last command to your install directory for houdini and execute the script inside off a shell 
	this will setup the environment varibles needed for the Resolver and then start houdini
optional
- in the launchHouWithResolver.bat you have the option to disable TF_DEBUG by commenting out the line. 

Linux 
