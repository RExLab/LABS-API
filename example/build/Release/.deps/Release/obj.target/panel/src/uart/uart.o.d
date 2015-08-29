cmd_Release/obj.target/panel/src/uart/uart.o := g++ '-D_LARGEFILE_SOURCE' '-D_FILE_OFFSET_BITS=64' '-DBUILDING_NODE_EXTENSION' -I/root/.node-gyp/0.12.1/src -I/root/.node-gyp/0.12.1/deps/uv/include -I/root/.node-gyp/0.12.1/deps/v8/include -I../node_modules/nan -I../src -I../src/uart -I../src/modbus -I../src/crc -I../src/timer  -fPIC -pthread -Wall -Wextra -Wno-unused-parameter -O3 -ffunction-sections -fdata-sections -fno-tree-vrp -fno-omit-frame-pointer -fno-rtti -fno-exceptions -MMD -MF ./Release/.deps/Release/obj.target/panel/src/uart/uart.o.d.raw  -c -o Release/obj.target/panel/src/uart/uart.o ../src/uart/uart.cc
Release/obj.target/panel/src/uart/uart.o: ../src/uart/uart.cc \
 ../src/uart/uart.h ../src/uart/../uc_libdefs.h
../src/uart/uart.cc:
../src/uart/uart.h:
../src/uart/../uc_libdefs.h:
