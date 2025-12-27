#!/bin/bash

# Setup script for code linting and formatting tools
# Run this script to install and configure development tools

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}🔧 BackRooms UE5.7 Linting Setup${NC}"
echo -e "${BLUE}===================================${NC}\n"

# Check if Homebrew is installed
if ! command -v brew &> /dev/null; then
    echo -e "${RED}❌ Homebrew not found. Please install Homebrew first:${NC}"
    echo -e "${YELLOW}   /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\"${NC}"
    exit 1
fi

echo -e "${GREEN}✅ Homebrew found${NC}"

# Install clang-format
echo -e "\n${BLUE}📦 Installing clang-format...${NC}"
if command -v clang-format &> /dev/null; then
    echo -e "${GREEN}✅ clang-format already installed ($(clang-format --version | head -n1))${NC}"
else
    brew install clang-format
    echo -e "${GREEN}✅ clang-format installed${NC}"
fi

# Install clang-tidy (comes with LLVM)
echo -e "\n${BLUE}📦 Installing clang-tidy...${NC}"
if command -v clang-tidy &> /dev/null; then
    echo -e "${GREEN}✅ clang-tidy already installed ($(clang-tidy --version | head -n1))${NC}"
else
    brew install llvm
    echo -e "${GREEN}✅ clang-tidy installed${NC}"
    
    # Add LLVM to PATH if needed
    LLVM_PATH="/opt/homebrew/opt/llvm/bin"
    if [ -d "$LLVM_PATH" ] && [[ ":$PATH:" != *":$LLVM_PATH:"* ]]; then
        echo -e "${YELLOW}⚠️  Adding LLVM to PATH. Add this to your shell profile:${NC}"
        echo -e "${YELLOW}   export PATH=\"$LLVM_PATH:\$PATH\"${NC}"
    fi
fi

# Verify configuration files
echo -e "\n${BLUE}📋 Verifying configuration files...${NC}"

if [ -f ".clang-format" ]; then
    echo -e "${GREEN}✅ .clang-format configuration found${NC}"
else
    echo -e "${RED}❌ .clang-format not found${NC}"
    exit 1
fi

if [ -f ".clang-tidy" ]; then
    echo -e "${GREEN}✅ .clang-tidy configuration found${NC}"
else
    echo -e "${RED}❌ .clang-tidy not found${NC}"
    exit 1
fi

if [ -f "pre-commit-hook.sh" ]; then
    echo -e "${GREEN}✅ Pre-commit hook script found${NC}"
else
    echo -e "${RED}❌ pre-commit-hook.sh not found${NC}"
    exit 1
fi

if [ -f "CODING_STANDARDS.md" ]; then
    echo -e "${GREEN}✅ Coding standards documentation found${NC}"
else
    echo -e "${RED}❌ CODING_STANDARDS.md not found${NC}"
    exit 1
fi

# Setup git hooks
echo -e "\n${BLUE}🪝 Setting up git hooks...${NC}"
if [ -d ".git" ]; then
    cp pre-commit-hook.sh .git/hooks/pre-commit
    chmod +x .git/hooks/pre-commit
    echo -e "${GREEN}✅ Pre-commit hook installed${NC}"
else
    echo -e "${YELLOW}⚠️  No .git directory found. Initialize git repository first.${NC}"
fi

# Test formatting on a sample file
echo -e "\n${BLUE}🧪 Testing configuration...${NC}"

# Find a source file to test
SAMPLE_FILE=$(find Source -name "*.cpp" -o -name "*.h" | head -n1)

if [ -n "$SAMPLE_FILE" ] && [ -f "$SAMPLE_FILE" ]; then
    echo -e "${BLUE}   Testing clang-format on: $SAMPLE_FILE${NC}"
    
    # Create backup
    cp "$SAMPLE_FILE" "${SAMPLE_FILE}.bak"
    
    # Test formatting (dry-run first)
    if clang-format --dry-run "$SAMPLE_FILE" &> /dev/null; then
        echo -e "${GREEN}   ✅ clang-format test passed${NC}"
    else
        echo -e "${RED}   ❌ clang-format test failed${NC}"
    fi
    
    # Restore backup
    mv "${SAMPLE_FILE}.bak" "$SAMPLE_FILE"
    
    # Test clang-tidy (basic check)
    echo -e "${BLUE}   Testing clang-tidy basic functionality...${NC}"
    if command -v clang-tidy &> /dev/null; then
        if clang-tidy --version &> /dev/null; then
            echo -e "${GREEN}   ✅ clang-tidy test passed${NC}"
        else
            echo -e "${YELLOW}   ⚠️  clang-tidy available but may need configuration${NC}"
        fi
    else
        echo -e "${YELLOW}   ⚠️  clang-tidy not in PATH${NC}"
    fi
else
    echo -e "${YELLOW}   ⚠️  No source files found to test${NC}"
fi

echo -e "\n${GREEN}🎉 Linting setup complete!${NC}"
echo -e "\n${BLUE}📚 Next steps:${NC}"
echo -e "${YELLOW}   1. Read CODING_STANDARDS.md for detailed guidelines${NC}"
echo -e "${YELLOW}   2. Format existing code: find Source -name \"*.cpp\" -o -name \"*.h\" | xargs clang-format -i${NC}"
echo -e "${YELLOW}   3. Run static analysis: clang-tidy Source/**/*.cpp -- -std=c++17 -ISource${NC}"
echo -e "${YELLOW}   4. Test pre-commit hook: git add . && git commit -m \"test\"${NC}"

echo -e "\n${GREEN}✅ Development environment ready!${NC}"