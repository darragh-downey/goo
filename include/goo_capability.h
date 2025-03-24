#ifndef GOO_CAPABILITY_H
#define GOO_CAPABILITY_H

/**
 * Goo Capability System Definitions
 * 
 * This header defines the capability types and constants used by both 
 * the compiler and runtime system. Capabilities provide a fine-grained
 * access control system for system resources.
 */

#include "goo_core.h"

// Capability attribute names
#define GOO_CAP_ATTR_REQUIRES  "requires"
#define GOO_CAP_ATTR_PROVIDES  "provides"
#define GOO_CAP_ATTR_REVOKES   "revokes"

// Capability operations
typedef enum {
    GOO_CAP_OP_CHECK,      // Check if a capability is present
    GOO_CAP_OP_GRANT,      // Grant a capability
    GOO_CAP_OP_REVOKE,     // Revoke a capability
    GOO_CAP_OP_CLONE,      // Clone a capability set
    GOO_CAP_OP_RESTRICT    // Restrict capabilities to a subset
} GooCapabilityOperation;

// Capability name mappings
typedef struct {
    const char* name;
    GooCapabilityType type;
} GooCapabilityMapping;

// Standard capability mappings
static const GooCapabilityMapping GOO_STANDARD_CAPABILITIES[] = {
    {"file", GOO_CAP_FILE_IO},
    {"network", GOO_CAP_NETWORK},
    {"process", GOO_CAP_PROCESS},
    {"memory", GOO_CAP_MEMORY},
    {"time", GOO_CAP_TIME},
    {"signal", GOO_CAP_SIGNAL},
    {"device", GOO_CAP_DEVICE},
    {"unsafe", GOO_CAP_UNSAFE},
    {"sandbox", GOO_CAP_SANDBOX},
    {"all", GOO_CAP_ALL},
    {NULL, GOO_CAP_NONE} // End marker
};

#endif // GOO_CAPABILITY_H 