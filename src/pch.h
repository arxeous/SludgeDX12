#pragma once
// Precompiled header file. Saves us a lot of time just having everything we need in one place

// get rid of windows APIs that arent used often and use the W string version of functions.
#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define DEBUG

// standard includes for directx 12
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <comdef.h>
#include "libs/d3d12x/d3dx12.h"
#include "errors.h"
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/version.h>


// std libs we use
#include <span>
#include <exception>
#include <utility>
#include <array>
#include <cassert>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <memory>
#include <fstream>
#include <queue>
#include <map>
#include <type_traits>
#include <cstdint>
#include <assert.h>



