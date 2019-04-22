//
// Created by artem on 22.04.19.
//

#include <iostream>
#include <sys/mman.h>
#include <cstring>
#include <sstream>


using namespace std;

// int function (n) = 2^n
unsigned char function[] = {
        0x55,
        0x48, 0x89, 0xe5,
        0x89, 0x7d, 0xec,
        0xc7, 0x45, 0xfc, 0x02, 0x00, 0x00, 0x00,
        0xc7, 0x45, 0xf4, 0x01, 0x00, 0x00, 0x00,
        0xc7, 0x45, 0xf8, 0x00, 0x00, 0x00, 0x00,
        0x8b, 0x45, 0xf8,
        0x3b, 0x45, 0xec,
        0x7d, 0x10,
        0x8b, 0x45, 0xf4,
        0x0f, 0xaf, 0x45, 0xfc,
        0x89, 0x45, 0xf4,
        0x83, 0x45, 0xf8, 0x01,
        0xeb, 0xe8,
        0x8b, 0x45, 0xf4,
        0x5d,
        0xc3
};

template<typename>
struct functionWrapper;

template<typename Ret, typename ... Args>
struct functionWrapper<Ret(Args...)> {
    void *functionData;
    size_t size;

    functionWrapper(const unsigned char *data, size_t size) : size(size) {
        functionData = mmap(nullptr, size, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANON, -1, 0);
        if (functionData == MAP_FAILED) {
            throw runtime_error("Can't map a function");
        }
        memcpy(functionData, data, size);
        if (mprotect(functionData, size, PROT_READ | PROT_EXEC)) {
            throw runtime_error("Can't change protections of function");
        }
    }

    ~functionWrapper() {
        if (munmap(functionData, size) == -1) {
            cerr << "Error occurred while unmapping function. " << errno;
        }
    }

    Ret operator()(Args ... args) const {
        return ((Ret (*)(Args...)) functionData)(args...);
    }
};

void modify(int a) {
    memcpy(function + 10, &a, 4);
}

void printUsage() {
    cerr << "Usage: jit <exponent value>" << endl;
    cerr << "       jit <base value> <exponent value>" << endl;
    cerr << "Returns base^exponent. Default base value = 2" << endl;
}

int strtoi(const string &str) {
    int n;
    istringstream iss(str);
    if (iss >> n) {
        if (!iss.good()) {
            return n;
        }
    }
    throw runtime_error(str + " can't be converted to int");
}

int main(int argc, char *argv[]) {
    if (argc > 3 || argc < 2) {
        printUsage();
        return 0;
    }
    int exp;
    try {
        if (argc == 3) {
            int base = strtoi(argv[1]);
            modify(base);
            exp = strtoi(argv[2]);
        } else {
            exp = strtoi(argv[1]);
        }
        if (exp < 0) {
            throw runtime_error("Exponent must be >= 0");
        }
    } catch (runtime_error &e) {
        cerr << e.what() << endl;
        return 0;
    }
    try {
        functionWrapper<int(int)> f(function, sizeof(function));
        cout << f.operator()(exp) << endl;
    } catch (runtime_error &e) {
        cerr << e.what() << ' ' << errno << endl;
    }
    return 0;
}