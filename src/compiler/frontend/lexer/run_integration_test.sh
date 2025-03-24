#!/bin/bash
set -e

# Colors for better output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}==== Goo Lexer Integration Test ====${NC}"

# Step 1: Build the Zig lexer
echo -e "\n${YELLOW}Building Zig lexer...${NC}"
cd zig
zig build
if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to build Zig lexer${NC}"
    exit 1
fi
cd ..

# Step 2: Build the integration test
echo -e "\n${YELLOW}Building integration test...${NC}"
make -f Makefile.integration
if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to build integration test${NC}"
    exit 1
fi

# Step 3: Run tokenizer test on sample file
echo -e "\n${YELLOW}Running tokenizer test on test_sample.goo...${NC}"
./test_lexer_integration test_sample.goo --tokenize-only
if [ $? -ne 0 ]; then
    echo -e "${RED}Tokenizer test failed${NC}"
    exit 1
fi

# Step 4: Create a simple parser for testing if it doesn't exist
echo -e "\n${YELLOW}Creating parser for integration testing...${NC}"
if [ ! -f parser.tab.c ]; then
    make -f Makefile.integration parser
    if [ $? -ne 0 ]; then
        echo -e "${RED}Failed to generate parser${NC}"
        exit 1
    fi
fi

# Step 5: Build the parser integration test
echo -e "\n${YELLOW}Building parser integration test...${NC}"
make -f Makefile.integration test_with_parser
if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to build parser integration test${NC}"
    exit 1
fi

# Step 6: Run parser integration test
echo -e "\n${YELLOW}Running parser integration test...${NC}"
./test_with_parser test_sample.goo
if [ $? -ne 0 ]; then
    echo -e "${RED}Parser integration test failed${NC}"
    exit 1
fi

# Step 7: Compare with Flex lexer if available
echo -e "\n${YELLOW}Comparing with Flex lexer...${NC}"
if [ -f flex_tokenizer ]; then
    echo -e "Flex lexer output:"
    ./flex_tokenizer test_sample.goo
    
    # Check for differences
    echo -e "\n${YELLOW}Checking for differences between lexers...${NC}"
    mkdir -p results
    ./flex_tokenizer test_sample.goo > results/flex_tokens.txt
    ./test_lexer_integration test_sample.goo --tokenize-only > results/zig_tokens.txt
    
    # Use diff to compare the outputs (ignoring line numbers which may differ)
    diff -y --suppress-common-lines results/flex_tokens.txt results/zig_tokens.txt > results/lexer_diff.txt
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}No significant differences found between lexers!${NC}"
    else
        echo -e "${YELLOW}Some differences found between lexers. Check results/lexer_diff.txt${NC}"
    fi
else
    echo -e "${YELLOW}Flex tokenizer not found. Skipping comparison.${NC}"
fi

echo -e "\n${GREEN}Integration test completed successfully!${NC}"
echo -e "${YELLOW}Next steps:${NC}"
echo "1. Update the main compiler build system to use the Zig lexer"
echo "2. Run the full compiler test suite with the new lexer"
echo "3. Update documentation to reflect the change" 