# unitree_slam_example

### Build examples
```bash
# Build and run
mkdir build				
cd build
cmake ..
make
export LD_LIBRARY_PATH=$PWD/../unitree_robotics/lib/$(uname -m):$LD_LIBRARY_PATH 
./demo_h1 eth0   #eth0 is the network interface name for the subnet 123.
```

