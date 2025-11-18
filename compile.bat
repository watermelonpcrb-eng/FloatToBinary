g++ -std=c++17 -DCPPHTTPLIB_NO_MMAP -D_WIN32_WINNT=0x0A00 server.cpp -o server.exe -lws2_32
