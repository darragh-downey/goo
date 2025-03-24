/**
 * type_error_codes.h
 * 
 * Error codes for the Goo type system
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#ifndef GOO_TYPE_ERROR_CODES_H
#define GOO_TYPE_ERROR_CODES_H

// Type mismatch errors
#define GOO_ERR_TYPE_MISMATCH          "E0001"
#define GOO_ERR_TYPE_INFERENCE         "E0002"
#define GOO_ERR_TYPE_CONDITION         "E0003"
#define GOO_ERR_TYPE_CHANNEL           "E0004"
#define GOO_ERR_TYPE_COMPARISON        "E0005"

// Declaration and reference errors
#define GOO_ERR_UNDEFINED_VARIABLE     "E0101"
#define GOO_ERR_DUPLICATE_VARIABLE     "E0102"
#define GOO_ERR_DUPLICATE_FUNCTION     "E0103"
#define GOO_ERR_UNDEFINED_FUNCTION     "E0104"
#define GOO_ERR_UNDEFINED_TYPE         "E0105"
#define GOO_ERR_DUPLICATE_TYPE         "E0106"
#define GOO_ERR_PARAMETER_TYPE         "E0107"

// Expression errors
#define GOO_ERR_INVALID_OPERANDS       "E0201"
#define GOO_ERR_INVALID_OPERAND        "E0202"
#define GOO_ERR_UNSUPPORTED_OPERATOR   "E0203"
#define GOO_ERR_NON_BOOLEAN_CONDITION  "E0204"
#define GOO_ERR_NON_REFERENCE          "E0205"
#define GOO_ERR_IDENTIFIER_RESOLUTION  "E0206"

// Function call errors
#define GOO_ERR_CALL_FUNCTION          "E0010"
#define GOO_ERR_NON_FUNCTION           "E0301"
#define GOO_ERR_ARGUMENT_COUNT         "E0302"
#define GOO_ERR_ARGUMENT_TYPE          "E0303"

// Statement errors
#define GOO_ERR_UNSUPPORTED_STATEMENT  "E0401"
#define GOO_ERR_UNSUPPORTED_EXPRESSION "E0402"

// Generic errors
#define GOO_ERR_GENERIC                "E9999"

#endif /* GOO_TYPE_ERROR_CODES_H */ 