package main

// This tests Go syntax with Goo extensions

// Simple function declaration
func add(a int, b int) int {
	return a + b
}

// Type declaration
type Point struct {
	x float64
	y float64
}

// Interface declaration
type Comparable interface {
	Compare(other interface{}) int
}

// Goo extension: compile-time function
func calc() int {
	// Will be implemented as a Goo extension
	return 0
}

// Main function
func main() {
	result := add(1, 2)
	println(result)
}
