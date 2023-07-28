Utils - how to build with docker / antelope


1 - configure cmake and ninja
cmake -S . -B ./build -G Ninja

2 - on docker container add ninja

apt-get update
apt-get install ninja-build
