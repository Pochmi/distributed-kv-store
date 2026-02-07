#!/bin/bash

# ========================================
# Distributed KV Store - Build Script
# Phase 3: Master-Slave Replication
# ========================================

set -e  # Exit on error
set -u  # Exit on undefined variable

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Log functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Display help
show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --help, -h       Show this help message"
    echo "  --clean, -c      Clean build directory before building"
    echo "  --release        Build in release mode"
    echo "  --debug          Build in debug mode (default)"
    echo "  --asan           Enable Address Sanitizer"
    echo "  --tsan           Enable Thread Sanitizer"
    echo "  --test           Build and run tests"
    echo "  --examples       Build examples"
    echo "  --install        Install after building"
    echo "  --verbose, -v    Verbose output"
    echo ""
    echo "Examples:"
    echo "  $0                # Debug build"
    echo "  $0 --release      # Release build"
    echo "  $0 --clean --asan # Clean build with address sanitizer"
    echo "  $0 --test         # Build and run tests"
}

# Parse command line arguments
CLEAN=false
BUILD_TYPE="Debug"
WITH_ASAN=false
WITH_TSAN=false
RUN_TESTS=false
BUILD_EXAMPLES=false
INSTALL=false
VERBOSE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -c|--clean)
            CLEAN=true
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --asan)
            WITH_ASAN=true
            shift
            ;;
        --tsan)
            WITH_TSAN=true
            shift
            ;;
        --test)
            RUN_TESTS=true
            shift
            ;;
        --examples)
            BUILD_EXAMPLES=true
            shift
            ;;
        --install)
            INSTALL=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        *)
            log_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Project information
PROJECT_NAME="Distributed KV Store"
PROJECT_VERSION="3.0.0"
BUILD_DIR="build"
THIRD_PARTY_DIR="third_party"

# Check dependencies
check_dependencies() {
    log_info "Checking dependencies..."
    
    # Check for CMake
    if ! command -v cmake &> /dev/null; then
        log_error "CMake not found. Please install CMake (minimum version 3.10)"
        exit 1
    fi
    
    # Check CMake version
    CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
    CMAKE_MAJOR=$(echo $CMAKE_VERSION | cut -d'.' -f1)
    CMAKE_MINOR=$(echo $CMAKE_VERSION | cut -d'.' -f2)
    
    if [ $CMAKE_MAJOR -lt 3 ] || ([ $CMAKE_MAJOR -eq 3 ] && [ $CMAKE_MINOR -lt 10 ]); then
        log_error "CMake version $CMAKE_VERSION is too old. Need at least 3.10"
        exit 1
    fi
    
    # Check for make/ninja
    if command -v ninja &> /dev/null; then
        GENERATOR="Ninja"
    elif command -v make &> /dev/null; then
        GENERATOR="Unix Makefiles"
    else
        log_error "No build system found. Install make or ninja"
        exit 1
    fi
    
    # Check for C++ compiler
    if ! command -v g++ &> /dev/null; then
        if command -v clang++ &> /dev/null; then
            log_warning "GCC not found, using clang++"
        else
            log_error "No C++ compiler found. Install g++ or clang++"
            exit 1
        fi
    fi
    
    log_success "Dependencies check passed"
}

# Clean build directory
clean_build() {
    if [ "$CLEAN" = true ]; then
        log_info "Cleaning build directory..."
        rm -rf "$BUILD_DIR"
        rm -rf bin lib  # Clean old style output directories
    fi
}

# Configure build
configure_build() {
    log_info "Configuring build with CMake..."
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Build CMake options
    CMAKE_OPTS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
    CMAKE_OPTS="$CMAKE_OPTS -G \"$GENERATOR\""
    
    if [ "$WITH_ASAN" = true ]; then
        CMAKE_OPTS="$CMAKE_OPTS -DWITH_ASAN=ON"
    fi
    
    if [ "$WITH_TSAN" = true ]; then
        CMAKE_OPTS="$CMAKE_OPTS -DWITH_TSAN=ON"
    fi
    
    if [ "$BUILD_EXAMPLES" = true ]; then
        CMAKE_OPTS="$CMAKE_OPTS -DBUILD_EXAMPLES=ON"
    fi
    
    if [ "$VERBOSE" = true ]; then
        CMAKE_OPTS="$CMAKE_OPTS -DCMAKE_VERBOSE_MAKEFILE=ON"
    fi
    
    # Run CMake
    if [ "$VERBOSE" = true ]; then
        eval "cmake $CMAKE_OPTS .."
    else
        eval "cmake $CMAKE_OPTS .." > /dev/null
    fi
    
    cd ..
    
    if [ $? -eq 0 ]; then
        log_success "Configuration successful"
    else
        log_error "CMake configuration failed"
        exit 1
    fi
}

# Build project
build_project() {
    log_info "Building project..."
    
    cd "$BUILD_DIR"
    
    # Get number of CPU cores
    if command -v nproc &> /dev/null; then
        CORES=$(nproc)
    else
        CORES=4
    fi
    
    # Build
    if [ "$VERBOSE" = true ]; then
        make -j$CORES
    else
        make -j$CORES > /dev/null
    fi
    
    BUILD_RESULT=$?
    cd ..
    
    if [ $BUILD_RESULT -eq 0 ]; then
        log_success "Build successful"
    else
        log_error "Build failed"
        exit 1
    fi
}

# Run tests
run_tests() {
    if [ "$RUN_TESTS" = true ]; then
        log_info "Running tests..."
        cd "$BUILD_DIR"
        
        if [ -f "bin/test_unit" ]; then
            log_info "Running unit tests..."
            ./bin/test_unit
        fi
        
        if [ -f "bin/test_integration" ]; then
            log_info "Running integration tests..."
            ./bin/test_integration
        fi
        
        cd ..
    fi
}

# Install
install_project() {
    if [ "$INSTALL" = true ]; then
        log_info "Installing project..."
        cd "$BUILD_DIR"
        sudo make install
        cd ..
        log_success "Installation completed"
    fi
}

# Show build summary
show_summary() {
    echo ""
    echo "========================================"
    echo "          BUILD SUMMARY"
    echo "========================================"
    echo "Project:    $PROJECT_NAME"
    echo "Version:    $PROJECT_VERSION"
    echo "Build Type: $BUILD_TYPE"
    echo "Sanitizers: $(if $WITH_ASAN; then echo -n "ASAN "; fi)$(if $WITH_TSAN; then echo -n "TSAN"; fi)"
    echo ""
    echo "Generated executables:"
    echo "  ./$BUILD_DIR/bin/kv_server    - Main server"
    echo "  ./$BUILD_DIR/bin/kv_client    - Client"
    echo "  ./$BUILD_DIR/bin/kv_admin     - Admin tool"
    
    if [ "$BUILD_EXAMPLES" = true ]; then
        echo ""
        echo "Examples:"
        echo "  ./$BUILD_DIR/bin/example_basic       - Basic usage"
        echo "  ./$BUILD_DIR/bin/example_replication - Replication demo"
    fi
    
    echo ""
    echo "Configuration files:"
    echo "  ./configs/master.json    - Master node config"
    echo "  ./configs/slave.json     - Slave node config"
    echo "  ./configs/client.json    - Client config"
    
    echo ""
    echo "Quick start:"
    echo "  1. Start master:  ./$BUILD_DIR/bin/kv_server --config configs/master.json"
    echo "  2. Start slave:   ./$BUILD_DIR/bin/kv_server --config configs/slave.json"
    echo "  3. Run client:    ./$BUILD_DIR/bin/kv_client --config configs/client.json"
    
    echo ""
    echo "Test scripts:"
    echo "  ./tests/scripts/start_replication_cluster.sh"
    echo "  ./tests/scripts/test_replication_basic.sh"
    
    echo "========================================"
}

# Main execution
main() {
    echo -e "${GREEN}"
    echo "╔══════════════════════════════════════════╗"
    echo "║   Distributed KV Store - Build System    ║"
    echo "║              Phase 3: Replication        ║"
    echo "╚══════════════════════════════════════════╝"
    echo -e "${NC}"
    
    # Check dependencies
    check_dependencies
    
    # Clean if requested
    clean_build
    
    # Configure and build
    configure_build
    build_project
    
    # Run tests if requested
    run_tests
    
    # Install if requested
    install_project
    
    # Show summary
    show_summary
}

# Run main function
main
