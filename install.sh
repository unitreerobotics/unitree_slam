WorkDir=$(cd $(dirname $0); pwd)
echo "WrokDir=$WorkDir"

Arch=$(uname -m)
echo "CPU Arch=$Arch"
set -e

mkdir -p /usr/local/lib/cmake

cp -r $WorkDir/unitree_robotics/include/* /usr/local/include
cp -r $WorkDir/unitree_robotics/lib/$Arch/* /usr/local/lib
cp -r $WorkDir/unitree_robotics/lib/$Arch/* /usr/local/lib/$Arch
cp -r $WorkDir/unitree_robotics/lib/cmake/* /usr/local/lib/cmake

echo "success"
