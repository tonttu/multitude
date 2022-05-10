# "source" this file to get nice environment variables in bash/zsh.

PWD=$(pwd)

export LD_LIBRARY_PATH=$PWD/lib:$LD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=$PWD/lib:$DYLD_LIBRARY_PATH

