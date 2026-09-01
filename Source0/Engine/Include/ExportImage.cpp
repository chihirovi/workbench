#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include "ExportImage.hpp"

void export_raw_radiance(std::string filename, float* data, uint32_t width, uint32_t height){
    if (!data) {
        std::cerr << "Error: data is null." << std::endl;
        return;
    }

    FILE* fp = std::fopen(filename.c_str(), "w+");
    if (!fp) {
        return;
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint32_t offset = (y*width+x)*3;
            std::fprintf(fp, "%.6f, %.6f, %.6f\n", data[offset+0], data[offset+1], data[offset+2]); // 小数6桁
        }
    }

    std::fclose(fp);
}

void export_tonemap_srgb(std::string filename, float* data, uint32_t width, uint32_t height){
    if (!data) {
        std::cerr << "Error: data is null." << std::endl;
        return;
    }

    FILE* fp = std::fopen(filename.c_str(), "w+");
    if (!fp) {
        return;
    }

    std::fprintf(fp, "P3\n");
    std::fprintf(fp, "%d %d\n", width, height);
    std::fprintf(fp, "255\n");

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint32_t offset = (y*width+x)*3;
            std::fprintf(fp, "%d %d %d ", 
                uint32_t(data[offset+0]*255.0), 
                uint32_t(data[offset+1]*255.0), 
                uint32_t(data[offset+2]*255.0)
            );
        }
        std::fprintf(fp, "\n");
    }

    std::fclose(fp);
}