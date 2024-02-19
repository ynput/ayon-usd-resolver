Ayon OpenUsd Resolver (uses Ayon cpp Api)

requiered:
  C++ Compiler
  Cmake
  GitHub public key setup ( this is because the submodules are linked via git@ )
Tested Platforms:

git clone --recurse-submodules git@github.com:ynput/ayon-usd-resolver.git

git submodule update --init --recursive

linux build ./build.sh or ./build.sh Debug ( for well debug build, also builds the testing applycatoin )
