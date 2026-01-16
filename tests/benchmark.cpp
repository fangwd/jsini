#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include "jsini.hpp"

extern "C" {
    #include "jsini.h"
}

int main() {
    const int N = 100000;
    std::string json = "[";
    for (int i = 0; i < N; ++i) {
        json += std::to_string(i);
        if (i < N - 1) json += ",";
    }
    json += "]";

    std::cout << "JSON size: " << json.size() << " bytes, Elements: " << N << "\n";

    // Benchmark C++ Wrapper
    {
        auto start = std::chrono::high_resolution_clock::now();
        jsini::Value root(json);
        long long sum = 0;
        for (int i = 0; i < N; ++i) {
            // internal implicit cast to int
            sum += (int)root[i]; 
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start;
        std::cout << "C++ Wrapper Time: " << diff.count() << " s (Sum: " << sum << ")\n";
    }

    // Benchmark Pure C
    {
        auto start = std::chrono::high_resolution_clock::now();
        jsini_value_t* root = jsini_parse_string(json.c_str(), json.length());
        jsini_array_t* arr = (jsini_array_t*)root;
        long long sum = 0;
        for (int i = 0; i < N; ++i) {
             jsini_value_t* val = jsini_aget(arr, i);
             sum += ((jsini_integer_t*)val)->data;
        }
        jsini_free(root);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start;
        std::cout << "Pure C Time:      " << diff.count() << " s (Sum: " << sum << ")\n";
    }

    return 0;
}
