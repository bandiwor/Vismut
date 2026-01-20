#include "ast_typing.h"

#include <stdbool.h>
#include <stdlib.h>

typedef struct {
    VValueType left_type;
    VValueType right_type;
    VValueType result_type;
} ASTBinaryOpRule;

typedef struct {
    ASTBinaryType op;
    const ASTBinaryOpRule *rules;
    int rules_count;
} ASTBinaryOpRules;

typedef enum {
    CAST_ALLOW_ALWAYS,
    CAST_ALLOW_EXPLICIT,
    CAST_ALLOW_NEVER,
} CastPermission;

typedef struct {
    VValueType from_type;
    VValueType to_type;
    CastPermission permission;
} CastRule;

#define BEGIN_BINARY_RULES(var_name) static const ASTBinaryOpRules var_name[] = {
#define END_RULES };
#define BINARY_OP(op_name, ...) \
    { \
        .op = AST_BINARY_##op_name, \
        .rules = (ASTBinaryOpRule[]){ __VA_ARGS__ }, \
        .rules_count = sizeof((ASTBinaryOpRule[]){ __VA_ARGS__ }) / sizeof(ASTBinaryOpRule) \
    },

#define BEGIN_CAST_RULES(var_name) static const CastRule var_name[] = {
#define CAST_RULE(from, to, perm) { from, to, perm },

#define RULE(r, l, res) { r, l, res }

#define INT VALUE_I64
#define FLOAT VALUE_F64
#define STR VALUE_STR

BEGIN_BINARY_RULES(binary_op_rules)
    BINARY_OP(
        ADD,
        RULE(INT, INT, INT),
        RULE(FLOAT, FLOAT, FLOAT),
        RULE(STR, STR, STR),
    )
    BINARY_OP(
        SUB,
        RULE(INT, INT, INT),
        RULE(FLOAT, FLOAT, FLOAT),
    )
    BINARY_OP(
        MUL,
        RULE(INT, INT, INT),
        RULE(FLOAT, FLOAT, FLOAT),
    )
    BINARY_OP(
        DIV,
        RULE(INT, INT, FLOAT),
        RULE(FLOAT, FLOAT, FLOAT),
    )
    BINARY_OP(
        INT_DIV,
        RULE(INT, INT, INT),
        RULE(FLOAT, FLOAT, INT),
    )
    BINARY_OP(
        POW,
        RULE(INT, INT, INT),
        RULE(FLOAT, FLOAT, FLOAT),
    )
    BINARY_OP(
        GREATER_THAN,
        RULE(INT, INT, INT),
        RULE(FLOAT, FLOAT, INT),
    )
    BINARY_OP(
        LESS_THAN,
        RULE(INT, INT, INT),
        RULE(FLOAT, FLOAT, INT),
    )
    BINARY_OP(
        GREATER_THAN_OR_EQUALS,
        RULE(INT, INT, INT),
        RULE(FLOAT, FLOAT, INT),
    )
    BINARY_OP(
        LESS_THAN_OR_EQUALS,
        RULE(INT, INT, INT),
        RULE(FLOAT, FLOAT, INT),
    )
END_RULES

BEGIN_CAST_RULES(cast_rules)
    CAST_RULE(INT, FLOAT, CAST_ALLOW_ALWAYS)
    CAST_RULE(FLOAT, INT, CAST_ALLOW_EXPLICIT)
END_RULES

VValueType GetBinaryOpResultType(const ASTBinaryType op, const VValueType left, const VValueType right) {
    for (int i = 0; i < (int) _countof(binary_op_rules); ++i) {
        const ASTBinaryOpRules rules = binary_op_rules[i];
        if (rules.op == op) {
            for (int rule_id = 0; rule_id < rules.rules_count; ++rule_id) {
                const ASTBinaryOpRule rule = rules.rules[rule_id];
                if (rule.left_type == left && rule.right_type == right) {
                    return rule.result_type;
                }
            }
            return VALUE_UNKNOWN;
        }
    }
    return VALUE_UNKNOWN;
}

VValueType GetUnaryOpResultType(const ASTUnaryType op, const VValueType operand) {
    switch (op) {
        case AST_UNARY_PLUS:
        case AST_UNARY_MINUS:
            if (operand == INT || operand == FLOAT) {
                return operand;
            }
            return VALUE_UNKNOWN;
        case AST_UNARY_LOGICAL_NOT:
            return INT;
        case AST_UNARY_BITWISE_NOT:
            return operand == INT ? INT : VALUE_UNKNOWN;
        default:
            return VALUE_UNKNOWN;
    }
}

static CastPermission GetCastPermission(const VValueType from_type, const VValueType to_type) {
    if (from_type == to_type) {
        return CAST_ALLOW_ALWAYS;
    }

    for (int i = 0; i < (int) _countof(cast_rules); ++i) {
        const CastRule rule = cast_rules[i];
        if (rule.from_type == from_type && rule.to_type == to_type) {
            return rule.permission;
        }
    }

    return CAST_ALLOW_NEVER;
}

bool IsCastAllowed(const VValueType from_type, const VValueType to_type, const bool is_explicit) {
    const CastPermission permission = GetCastPermission(from_type, to_type);
    switch (permission) {
        case CAST_ALLOW_ALWAYS:
            return true;
        case CAST_ALLOW_EXPLICIT:
            return is_explicit;
        case CAST_ALLOW_NEVER:
        default:
            return false;
    }
}

VValueType FindCommonType(const VValueType type_1, const VValueType type_2) {
    if (type_1 == type_2) return type_1;
    if (IsCastAllowed(type_1, type_2, false)) {
        return type_2;
    }
    if (IsCastAllowed(type_2, type_1, false)) {
        return type_1;
    }
    return VALUE_UNKNOWN;
}
