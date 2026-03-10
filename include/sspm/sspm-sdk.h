#pragma once

/**
 * @file sspm-sdk.h
 * @brief Main entry point for the ShioSakura Package Manager (SSPM) Module SDK.
 * 
 * To develop a module or backend for SSPM:
 * 1. Include this header.
 * 2. Inherit from sspm::IModule or sspm::Backend.
 * 3. Implement the required virtual methods.
 * 4. Export the factory functions (create_module/destroy_module).
 * 
 * @version 4.0.0-Sakura
 */

#include "sspm/module.h"
#include "sspm/backend.h"
#include "sspm/log.h"

// Macro to simplify exporting factory functions for shared libraries
#ifdef _WIN32
  #define SSPM_EXPORT __declspec(dllexport)
#else
  #define SSPM_EXPORT __attribute__((visibility("default")))
#endif

#define SSPM_REGISTER_MODULE(Class) \
    extern "C" SSPM_EXPORT sspm::IModule* create_module() { \
        return new Class(); \
    } \
    extern "C" SSPM_EXPORT void destroy_module(sspm::IModule* m) { \
        delete m; \
    }

#define SSPM_REGISTER_BACKEND(Class) \
    extern "C" SSPM_EXPORT sspm::Backend* create_backend() { \
        return new Class(); \
    } \
    extern "C" SSPM_EXPORT void destroy_backend(sspm::Backend* b) { \
        delete b; \
    }
