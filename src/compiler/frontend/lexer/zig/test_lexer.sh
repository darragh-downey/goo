#!/bin/bash

# Build the lexer first
echo "Building lexer..."
zig build

# Sample Goo code
SAMPLE_CODE="sample.goo"

echo "package main

import \"fmt\"

func main() {
    // This is a comment
    var x := 42
    var y float64 = 3.14
    var s string = \"Hello, world!\"
    var b bool = true
    
    /* This is a
       multiline comment */
       
    if x > 10 && y < 5.0 {
        fmt.Println(s)
    } else {
        fmt.Println(\"x is too small or y is too large\")
    }
    
    // Test range literal
    for i := 1..10 {
        fmt.Println(i)
    }
    
    // Test operators
    result := (x + 10) * (y - 1.0) / 2
    
    // Test vector types and operations
    var vec simd.Vector(4, float32)
    vec = vec + 1.0
}" > $SAMPLE_CODE

echo "Running lexer test on sample code..."
./zig-out/bin/lexer_test $SAMPLE_CODE

echo "Done!" 