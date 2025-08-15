# Agent Guidelines for ofxSomPalette

## Build Commands
- **Build examples**: `cd example_PaletteFromAudio && make` or `cd example_ContinuousPalettesFromAudio && make`
- **Clean build**: `make clean && make`
- **Xcode build**: Open `.xcodeproj` files in example directories
- **Run examples**: `cd example_*/bin && ./example_*`

## Code Style
- **Headers**: Use `#pragma once`, include OpenFrameworks (`ofMain.h`) first, then addon dependencies
- **Naming**: Classes use PascalCase (`SomPalette`), variables/functions use camelCase (`instanceData`)
- **Types**: Use modern C++ (`std::array`, `std::unique_ptr`), prefer `using` for type aliases
- **Initialization**: Use member initializer lists in constructors with braces `{ value }`
- **Threading**: Inherit from `ofThread`, use `ofThreadChannel` for inter-thread communication
- **Constants**: Use `static constexpr` for compile-time constants
- **Memory**: Use RAII, smart pointers for dynamic allocation
- **OpenFrameworks**: Follow OF conventions (ofColor, ofPixels, ofTexture, ofFbo)
- **Error handling**: Check allocation status (`isAllocated()`) before using textures/FBOs
- **File paths**: Use `ofFilePath::getUserHomeDir()` and proper path concatenation

## Dependencies
- Requires `ofxSelfOrganizingMap` addon (specified in `addon_config.mk`)
- Built for OpenFrameworks v0.12+