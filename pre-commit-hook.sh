#!/bin/bash

# Pre-commit hook for Unreal Engine C++ code formatting and linting
# Place this file in .git/hooks/pre-commit (without .sh extension) and make it executable

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🔧 Running pre-commit checks...${NC}"

# Check if clang-format is available
if ! command -v clang-format &> /dev/null; then
    echo -e "${YELLOW}⚠️  clang-format not found. Install with: brew install clang-format${NC}"
    echo -e "${YELLOW}   Skipping code formatting...${NC}"
    FORMAT_AVAILABLE=false
else
    FORMAT_AVAILABLE=true
fi

# Check if clang-tidy is available
if ! command -v clang-tidy &> /dev/null; then
    echo -e "${YELLOW}⚠️  clang-tidy not found. Install with: brew install llvm${NC}"
    echo -e "${YELLOW}   Skipping static analysis...${NC}"
    TIDY_AVAILABLE=false
else
    TIDY_AVAILABLE=true
fi

# Get list of C++ files that are staged for commit
CPP_FILES=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|h|hpp)$' || true)

if [ -z "$CPP_FILES" ]; then
    echo -e "${GREEN}✅ No C++ files to check${NC}"
    exit 0
fi

echo -e "${BLUE}📂 Found C++ files to check:${NC}"
echo "$CPP_FILES" | sed 's/^/   - /'

NEED_RESTAGE=false

# 1. Code Formatting with clang-format
if [ "$FORMAT_AVAILABLE" = true ]; then
    echo -e "\n${BLUE}🎨 Running clang-format...${NC}"
    
    for FILE in $CPP_FILES; do
        if [ -f "$FILE" ]; then
            # Format the file and check if it changed
            ORIGINAL_CONTENT=$(cat "$FILE")
            clang-format -i "$FILE"
            NEW_CONTENT=$(cat "$FILE")
            
            if [ "$ORIGINAL_CONTENT" != "$NEW_CONTENT" ]; then
                echo -e "${YELLOW}   📝 Formatted: $FILE${NC}"
                NEED_RESTAGE=true
            fi
        fi
    done
    
    if [ "$NEED_RESTAGE" = true ]; then
        echo -e "${YELLOW}⚠️  Some files were reformatted. Please review and re-add them:${NC}"
        echo -e "${YELLOW}   git add <files> && git commit${NC}"
        exit 1
    else
        echo -e "${GREEN}   ✅ All files properly formatted${NC}"
    fi
fi

# 2. Static Analysis with clang-tidy (only check, don't fix)
if [ "$TIDY_AVAILABLE" = true ]; then
    echo -e "\n${BLUE}🔍 Running clang-tidy static analysis...${NC}"
    
    TIDY_ISSUES=false
    
    # Only check source files in our project directories
    SOURCE_FILES=$(echo "$CPP_FILES" | grep -E '^Source/.*\.(cpp|h)$' || true)
    
    if [ -n "$SOURCE_FILES" ]; then
        for FILE in $SOURCE_FILES; do
            if [ -f "$FILE" ]; then
                echo -e "   🔎 Checking: $FILE"
                
                # Run clang-tidy and capture output
                if ! clang-tidy "$FILE" -- -std=c++17 -I"Source" 2>/dev/null; then
                    echo -e "${RED}   ❌ Issues found in: $FILE${NC}"
                    TIDY_ISSUES=true
                else
                    echo -e "${GREEN}   ✅ Clean: $FILE${NC}"
                fi
            fi
        done
        
        if [ "$TIDY_ISSUES" = true ]; then
            echo -e "\n${YELLOW}⚠️  clang-tidy found issues. Run this command to see details:${NC}"
            echo -e "${YELLOW}   clang-tidy Source/**/*.cpp -- -std=c++17 -I\"Source\"${NC}"
            echo -e "${YELLOW}   Fix issues or use --no-verify to bypass checks${NC}"
            exit 1
        else
            echo -e "${GREEN}   ✅ All files pass static analysis${NC}"
        fi
    else
        echo -e "${BLUE}   ℹ️  No source files to analyze${NC}"
    fi
fi

# 3. Basic file checks
echo -e "\n${BLUE}📋 Running basic file checks...${NC}"

for FILE in $CPP_FILES; do
    if [ -f "$FILE" ]; then
        # Check for trailing whitespace
        if grep -q '[[:space:]]$' "$FILE"; then
            echo -e "${RED}   ❌ Trailing whitespace found in: $FILE${NC}"
            exit 1
        fi
        
        # Check for tabs in source files (except Makefiles)
        if [[ "$FILE" =~ \.(cpp|h|hpp)$ ]] && grep -q $'\t' "$FILE"; then
            # This is acceptable for our .clang-format config that uses tabs
            continue
        fi
        
        # Check file ends with newline
        if [ -s "$FILE" ] && [ "$(tail -c1 "$FILE" | wc -l)" -eq 0 ]; then
            echo -e "${RED}   ❌ File doesn't end with newline: $FILE${NC}"
            exit 1
        fi
    fi
done

echo -e "${GREEN}   ✅ All files pass basic checks${NC}"

echo -e "\n${GREEN}🎉 All pre-commit checks passed!${NC}"
echo -e "${GREEN}   Ready to commit...${NC}"

exit 0