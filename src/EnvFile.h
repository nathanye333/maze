#pragma once

#include <string>
#include <string_view>

// Loads KEY=VALUE pairs from a .env file into the process environment.
// Existing environment variables are not overwritten.
// Returns true if a file was found and read (even if empty).
bool loadEnvFile(std::string_view path);

// Tries common locations relative to the current working directory and returns
// the path that was loaded, or empty if none were found.
std::string loadEnvFromDefaultLocations();
