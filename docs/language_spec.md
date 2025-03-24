# Goo Language Specification (v0.2.0)

This document describes the syntax and semantics of the Goo programming language.

## Table of Contents

1. [Introduction](#introduction)
2. [Lexical Elements](#lexical-elements)
3. [Types](#types)
4. [Declarations](#declarations)
5. [Expressions](#expressions)
6. [Statements](#statements)
7. [Packages](#packages)
8. [Program Structure](#program-structure)

## Introduction

Goo is a statically typed, compiled programming language that aims to combine the simplicity of Go with the safety features of Rust, the build-time capabilities of Zig, and the concurrency model of Erlang.

## Lexical Elements

### Comments

Goo supports two forms of comments:
- Line comments start with `//` and continue until the end of the line.
- Block comments start with `/*` and end with `*/`. Block comments can be nested.

```go
// This is a line comment

/* This is a block comment
   that spans multiple lines */

/* This is /* a nested */ block comment */
```

### Tokens

The following classes of tokens are defined:
- Identifiers
- Keywords
- Operators and punctuation
- Literals (integer, floating-point, string, boolean)

### Identifiers

Identifiers name program entities such as variables, types, and functions. An identifier is a sequence of one or more letters and digits, with the first character being a letter. Underscore (`_`) is considered a letter.

```go
x
_y
longIdentifier
snake_case_identifier
```

### Keywords

The following keywords are reserved and may not be used as identifiers:

```
break     default   func      interface  select
case      defer     go        map        struct
chan      else      goto      package    switch
const     fallthrough if        range      type
continue  for       import    return     var
```

### Operators and Punctuation

The following character sequences represent operators and punctuation:

```
+    &     +=    &=     &&    ==    !=    (    )
-    |     -=    |=     ||    <     <=    [    ]
*    ^     *=    ^=     <-    >     >=    {    }
/    <<    /=    <<=    ++    =     :=    ,    ;
%    >>    %=    >>=    --    !     ...   .    :
```

### Literals

#### Integer Literals

Integer literals can be decimal, octal, or hexadecimal:

```go
42        // decimal
0600      // octal
0xBadFace // hexadecimal
```

#### Floating-Point Literals

Floating-point literals have an integer part, a decimal point, and a fractional part:

```go
0.
72.40
2.71828
```

#### String Literals

String literals are enclosed in double quotes:

```go
"hello world"
"multi-line
string"
```

Raw string literals are enclosed in backticks and can contain newlines:

```go
`raw string
with multiple lines`
```

#### Boolean Literals

Boolean literals are `true` and `false`.

## Types

### Basic Types

Goo provides the following basic types:

- Boolean types: `bool`
- Numeric types:
  - `int`, `int8`, `int16`, `int32`, `int64`
  - `uint`, `uint8`, `uint16`, `uint32`, `uint64`
  - `float32`, `float64`
- String type: `string`

### Composite Types

Composite types are constructed using type literals:

- Array types: `[n]T`
- Slice types: `[]T`
- Struct types: `struct { ... }`
- Map types: `map[K]V`
- Interface types: `interface { ... }`

### Type Declarations

A type declaration defines a new named type:

```go
type T underlying_type
```

## Declarations

### Variable Declarations

Variables can be declared using the `var` keyword:

```go
var x int
var y int = 42
var z = 42 // Type inferred
```

### Short Variable Declarations

Inside functions, short variable declarations can be used:

```go
x := 42
```

### Constant Declarations

Constants are declared using the `const` keyword:

```go
const Pi = 3.14159
const (
    StatusOK = 200
    StatusNotFound = 404
)
```

### Function Declarations

Functions are declared using the `func` keyword:

```go
func add(x int, y int) int {
    return x + y
}

// Multiple return values
func swap(x, y int) (int, int) {
    return y, x
}
```

## Expressions

### Operands

Operands denote the elementary values in an expression:
- Literals
- Variables
- Function calls
- Parenthesized expressions

### Operators

Goo provides the usual arithmetic, logical, and comparison operators.

#### Arithmetic Operators

`+`, `-`, `*`, `/`, `%`

#### Comparison Operators

`==`, `!=`, `<`, `<=`, `>`, `>=`

#### Logical Operators

`&&`, `||`, `!`

### Function Calls

A function call invokes a function with the given arguments:

```go
result := add(1, 2)
```

## Statements

### If Statements

If statements specify conditional execution of statements:

```go
if x > 0 {
    return x
} else if x < 0 {
    return -x
} else {
    return 0
}
```

### For Statements

For statements provide iteration:

```go
// Traditional for loop
for i := 0; i < 10; i++ {
    sum += i
}

// While loop
for condition {
    // ...
}

// Infinite loop
for {
    // ...
}
```

### Return Statements

Return statements specify the values to be returned from a function:

```go
return
return x
return x, y
```

## Packages

### Package Declaration

Every Goo source file belongs to a package:

```go
package math
```

### Import Declaration

Import declarations specify packages to be imported:

```go
import "fmt"
import (
    "math"
    "strings"
)
```

## Program Structure

A Goo program consists of one or more source files. Each source file begins with a package declaration, followed by import declarations, followed by declarations of functions, variables, constants, and types.

The entry point of a Goo program is the `main` function in the `main` package:

```go
package main

import "fmt"

func main() {
    fmt.Println("Hello, world!")
}
```

## Standard Library

Goo will provide a comprehensive standard library including packages for:
- I/O operations
- String manipulation
- Mathematical functions
- Networking
- Concurrency
- And more

*Note: This specification is a work in progress and subject to change as the language evolves.* 