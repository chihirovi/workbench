#pragma once
#include <iostream>
#include <fstream>
#include <string>

void export_raw_radiance(std::string filename, float* data, uint32_t width, uint32_t height);
void export_tonemap_srgb(std::string filename, float* data, uint32_t width, uint32_t height);