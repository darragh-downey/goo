#!/bin/bash
# Script to test a single file with both lexers and compare results

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check for the filename argument
if [ "$#" -lt 1 ]; then
    echo "Usage: $0 <file.goo> [options]"
    echo "Options:"
    echo "  --debug    Enable debug output"
    echo "  --verbose  Show detailed token information"
    exit 1
fi

FILE="$1"
shift

# Check if the file exists
if [ ! -f "$FILE" ]; then
    echo -e "${RED}Error: File '$FILE' does not exist${NC}"
    exit 1
fi

OPTIONS=""
for arg in "$@"; do
    OPTIONS="$OPTIONS $arg"
done

# Add current directory to PATH if needed
if [ ! -f "./test_lexer_selection" ]; then
    echo -e "${YELLOW}Building test program...${NC}"
    make -f Makefile.integration test_lexer_selection
    if [ $? -ne 0 ]; then
        echo -e "${RED}Failed to build test program${NC}"
        exit 1
    fi
fi

echo -e "${BLUE}=== Testing file: $FILE ===${NC}"

# Run test with Zig lexer
echo -e "${YELLOW}Running with Zig lexer...${NC}"
USE_ZIG_LEXER=1 ./test_lexer_selection "$FILE" --zig-only $OPTIONS > zig_output.log
ZIG_EXIT_CODE=$?

if [ $ZIG_EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}✓ Zig lexer processed the file successfully${NC}"
else
    echo -e "${RED}✗ Zig lexer failed with exit code $ZIG_EXIT_CODE${NC}"
fi

# Run test with Flex lexer
echo -e "${YELLOW}Running with Flex lexer...${NC}"
USE_ZIG_LEXER=0 ./test_lexer_selection "$FILE" --flex-only $OPTIONS > flex_output.log
FLEX_EXIT_CODE=$?

if [ $FLEX_EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}✓ Flex lexer processed the file successfully${NC}"
else
    echo -e "${RED}✗ Flex lexer failed with exit code $FLEX_EXIT_CODE${NC}"
fi

# Compare the outputs
echo -e "${YELLOW}Comparing outputs...${NC}"
diff -u flex_output.log zig_output.log > diff_output.log
DIFF_EXIT_CODE=$?

if [ $DIFF_EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}✓ Outputs are identical${NC}"
    # Clean up if outputs match
    rm -f flex_output.log zig_output.log diff_output.log
else
    echo -e "${RED}✗ Outputs differ${NC}"
    echo -e "Differences saved to ${BLUE}diff_output.log${NC}"
    echo -e "Individual outputs: ${BLUE}flex_output.log${NC} and ${BLUE}zig_output.log${NC}"
fi

# Run the direct comparison
echo -e "${YELLOW}Running direct token comparison...${NC}"
USE_ZIG_LEXER=1 ./test_lexer_selection "$FILE" --compare $OPTIONS > compare_output.log

if grep -q "Token streams are identical" compare_output.log; then
    echo -e "${GREEN}✓ Token streams match exactly${NC}"
    rm -f compare_output.log
else
    echo -e "${RED}✗ Token streams differ${NC}"
    echo -e "Comparison details saved to ${BLUE}compare_output.log${NC}"
fi

# Print a summary
echo
echo -e "${BLUE}=== Summary ===${NC}"

if [ $ZIG_EXIT_CODE -eq 0 ] && [ $FLEX_EXIT_CODE -eq 0 ] && [ $DIFF_EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}All tests passed! Both lexers produce identical output.${NC}"
    exit 0
else
    echo -e "${RED}Tests failed. Please check the log files for details.${NC}"
    exit 1
fi 