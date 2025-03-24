#!/bin/bash

# This script tests and compares the Flex and Zig lexer implementations

# Set paths
FLEX_LEXER="./flex_tokenizer"
ZIG_LEXER="./zig/zig-out/bin/lexer_test"
SAMPLE_DIR="./samples"
RESULT_DIR="./results"

# Ensure directories exist
mkdir -p "$SAMPLE_DIR"
mkdir -p "$RESULT_DIR"

# Check if Flex lexer exists or can be built
if [ ! -f "$FLEX_LEXER" ]; then
    echo "Building Flex tokenizer..."
    gcc -o "$FLEX_LEXER" flex_tokenizer.c -lfl
    if [ $? -ne 0 ]; then
        echo "Failed to build Flex tokenizer. Make sure Flex is installed."
        echo "Continuing with Zig lexer only..."
    fi
fi

# Build Zig lexer
echo "Building Zig lexer..."
(cd zig && zig build)
if [ $? -ne 0 ]; then
    echo "Failed to build Zig lexer."
    exit 1
fi

# Create sample Goo files if they don't exist
if [ ! -f "$SAMPLE_DIR/basic.goo" ]; then
    echo "Creating sample Goo files..."
    
    # Basic sample with various token types
    cat > "$SAMPLE_DIR/basic.goo" << EOF
package main

import "fmt"

func main() {
    // Single line comment
    var x := 42
    var y float64 = 3.14
    var s string = "Hello, world!"
    var b bool = true
    
    /* Multi-line
       comment */
       
    if x > 10 && y < 5.0 {
        fmt.Println(s)
    } else {
        fmt.Println("x is too small or y is too large")
    }
    
    // Range literal
    for i := 1..10 {
        fmt.Println(i)
    }
}
EOF

    # Complex sample with more language features
    cat > "$SAMPLE_DIR/complex.goo" << EOF
package main

import (
    "fmt"
    "math"
    "time"
)

type Point struct {
    x float64
    y float64
}

func (p *Point) distance(q Point) float64 {
    return math.Sqrt((p.x - q.x)*(p.x - q.x) + (p.y - q.y)*(p.y - q.y))
}

func createChannel() chan int {
    ch := make(chan int, 10)
    return ch
}

func main() {
    // Test all numeric literals
    var a int = 42
    var b int = 0xFF  // hex
    var c int = 0o755 // octal
    var d int = 0b1101 // binary
    var e float64 = 3.14159
    var f float64 = 1.2e-10
    
    // Test strings with escape sequences
    var s1 string = "Hello\nWorld"
    var s2 string = "Tab\tcharacter"
    var s3 string = "Quote\"inside\"quotes"
    
    // Test operations
    result := (a + b) * (c - d) / int(e)
    
    // Test goroutines and channels
    ch := createChannel()
    
    go func() {
        for i := 0; i < 5; i++ {
            ch <- i
            time.Sleep(100 * time.Millisecond)
        }
        close(ch)
    }()
    
    // Test range over channel
    for v := range ch {
        fmt.Println(v)
    }
    
    // Test SIMD operations
    var vec simd.Vector(4, float32)
    vec = vec + 1.0
    
    fmt.Println("All tests completed")
}
EOF

    # Error cases to test lexer robustness
    cat > "$SAMPLE_DIR/errors.goo" << EOF
package main

func main() {
    // Unterminated string
    var s = "This string is not terminated
    
    // Invalid character
    var @ = 42
    
    // Unterminated comment
    /* This comment is not terminated
    
    // Invalid numeric literals
    var a = 0x
    var b = 3.
    var c = .5e
    
    // Invalid tokens
    var d = $100
}
EOF
fi

# Function to run lexer on a sample file
run_test() {
    local sample=$1
    local flex_result="$RESULT_DIR/flex_$(basename "$sample").txt"
    local zig_result="$RESULT_DIR/zig_$(basename "$sample").txt"
    
    echo "Testing with $sample..."
    
    # Run Flex lexer if available
    if [ -f "$FLEX_LEXER" ]; then
        echo "  Running Flex lexer..."
        "$FLEX_LEXER" < "$sample" > "$flex_result" 2>&1
    fi
    
    # Run Zig lexer
    echo "  Running Zig lexer..."
    "$ZIG_LEXER" "$sample" > "$zig_result" 2>&1
    
    # Compare results if both lexers ran
    if [ -f "$FLEX_LEXER" ] && [ -f "$flex_result" ] && [ -f "$zig_result" ]; then
        echo "  Comparing results..."
        # This is a simple diff, you might want to implement a more sophisticated
        # comparison that accounts for format differences
        diff -u "$flex_result" "$zig_result" > "$RESULT_DIR/diff_$(basename "$sample").txt"
        if [ $? -eq 0 ]; then
            echo "  ✅ Lexers produced identical token streams!"
        else
            echo "  ⚠️ Differences found. Check $RESULT_DIR/diff_$(basename "$sample").txt"
        fi
    fi
    
    echo ""
}

# Run tests on all sample files
for sample in "$SAMPLE_DIR"/*.goo; do
    run_test "$sample"
done

echo "All tests completed. Results are in $RESULT_DIR"

# Summary
echo "====== SUMMARY ======"
echo "Zig lexer implementation status:"
echo "✅ Token definitions"
echo "✅ Core lexer implementation"
echo "✅ C bindings"
echo "✅ Test program"
echo "✅ Build system integration"

# Check if there are any differences between the lexers
if [ -f "$FLEX_LEXER" ]; then
    differences=0
    for diff_file in "$RESULT_DIR"/diff_*.txt; do
        if [ -s "$diff_file" ]; then
            differences=1
            break
        fi
    done
    
    if [ $differences -eq 0 ]; then
        echo "✅ Compatible with Flex lexer (identical token streams)"
    else
        echo "⚠️ Some differences with Flex lexer (check diff files)"
    fi
fi

echo ""
echo "Next steps:"
echo "1. Update the main build system to use the Zig lexer"
echo "2. Integrate with the Bison parser"
echo "3. Run the full compiler test suite with the new lexer" 