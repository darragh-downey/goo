#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/goo_runtime.h"
#include "../../include/goo_capability.h"
#include "../ast/ast.h"
#include "../ast/symbol.h"
#include "../ast/type.h"
#include "codegen.h"

// Generates code for capability checking
void codegen_capability_check(CodegenContext* ctx, int capability_type) {
    // Generate code to check if the current goroutine has the required capability
    char* check_code = malloc(256);
    sprintf(check_code, 
            "if (!goo_capability_check(goo_runtime_get_current_caps(), %d)) {\n"
            "    goo_runtime_panic(\"Missing required capability\");\n"
            "    return;\n"
            "}\n", 
            capability_type);
    
    codegen_emit_raw(ctx, check_code);
    free(check_code);
}

// Generates code for granting a capability to a goroutine
void codegen_capability_grant(CodegenContext* ctx, int capability_type, const char* data_expr) {
    // Generate code to add a capability to the current goroutine's set
    char* grant_code = malloc(512);
    
    if (data_expr) {
        sprintf(grant_code,
                "{\n"
                "    GooCapabilitySet* caps = goo_runtime_get_current_caps();\n"
                "    if (!caps) {\n"
                "        caps = goo_capability_set_create();\n"
                "        goo_runtime_set_current_caps(caps);\n"
                "    }\n"
                "    goo_capability_add(caps, %d, (void*)(%s));\n"
                "}\n",
                capability_type, data_expr);
    } else {
        sprintf(grant_code,
                "{\n"
                "    GooCapabilitySet* caps = goo_runtime_get_current_caps();\n"
                "    if (!caps) {\n"
                "        caps = goo_capability_set_create();\n"
                "        goo_runtime_set_current_caps(caps);\n"
                "    }\n"
                "    goo_capability_add(caps, %d, NULL);\n"
                "}\n",
                capability_type);
    }
    
    codegen_emit_raw(ctx, grant_code);
    free(grant_code);
}

// Generates code for revoking a capability from a goroutine
void codegen_capability_revoke(CodegenContext* ctx, int capability_type) {
    // Generate code to remove a capability from the current goroutine's set
    char* revoke_code = malloc(256);
    sprintf(revoke_code,
            "{\n"
            "    GooCapabilitySet* caps = goo_runtime_get_current_caps();\n"
            "    if (caps) {\n"
            "        goo_capability_remove(caps, %d);\n"
            "    }\n"
            "}\n",
            capability_type);
    
    codegen_emit_raw(ctx, revoke_code);
    free(revoke_code);
}

// Processes a capability attribute in the AST
void codegen_process_capability_attr(CodegenContext* ctx, ASTNode* attr, ASTNode* func_decl) {
    if (!attr || !func_decl || attr->type != AST_ATTRIBUTE) {
        return;
    }
    
    // Check if this is a capability attribute
    if (strcmp(attr->attr.name, GOO_CAP_ATTR_REQUIRES) != 0) {
        return;
    }
    
    // Get the capability name
    if (attr->attr.args->type != AST_STRING_LITERAL) {
        fprintf(stderr, "Error: capability attribute requires a string literal\n");
        return;
    }
    
    const char* cap_name = attr->attr.args->string_literal.value;
    
    // Map capability name to ID
    int cap_id = codegen_get_capability_id(cap_name);
    if (cap_id == -1) {
        fprintf(stderr, "Error: unknown capability type: %s\n", cap_name);
        return;
    }
    
    // Add capability check at the start of the function body
    ASTNode* body = func_decl->function.body;
    if (body && body->type == AST_BLOCK) {
        // Insert capability check at the beginning of the function
        codegen_capability_check(ctx, cap_id);
    }
}

// Generate code for spawning a goroutine with capability inheritance
void codegen_goroutine_spawn_with_caps(CodegenContext* ctx, 
                                      const char* func_ptr, 
                                      const char* arg_ptr, 
                                      bool inherit_caps) {
    if (inherit_caps) {
        // Clone the current capability set and pass it to the new goroutine
        codegen_emit_raw(ctx, 
                        "{\n"
                        "    GooCapabilitySet* current_caps = goo_runtime_get_current_caps();\n"
                        "    GooCapabilitySet* new_caps = NULL;\n"
                        "    if (current_caps) {\n"
                        "        new_caps = goo_capability_set_clone(current_caps);\n"
                        "    }\n"
                        "    goo_goroutine_spawn_with_caps(");
        codegen_emit_raw(ctx, func_ptr);
        codegen_emit_raw(ctx, ", ");
        codegen_emit_raw(ctx, arg_ptr);
        codegen_emit_raw(ctx, ", new_caps);\n"
                        "}\n");
    } else {
        // Spawn a goroutine with no capabilities
        codegen_emit_raw(ctx, "goo_goroutine_spawn(");
        codegen_emit_raw(ctx, func_ptr);
        codegen_emit_raw(ctx, ", ");
        codegen_emit_raw(ctx, arg_ptr);
        codegen_emit_raw(ctx, ");\n");
    }
}

// Map a capability name to its runtime ID
int codegen_get_capability_id(const char* name) {
    if (!name) return -1;
    
    // Search in the standard capabilities mapping
    for (int i = 0; GOO_STANDARD_CAPABILITIES[i].name != NULL; i++) {
        if (strcmp(name, GOO_STANDARD_CAPABILITIES[i].name) == 0) {
            return GOO_STANDARD_CAPABILITIES[i].type;
        }
    }
    
    return -1;
} 