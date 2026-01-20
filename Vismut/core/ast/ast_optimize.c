//
// Created by kir on 26.12.2025.
//

#include "ast_optimize.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>

#include "ast.h"
#include "../errors/errors.h"

typedef struct {
    Arena *arena;
} SimpleOptimizationsContext;

#define SAFE_SIMPLE_OPTIMIZATIONS(ctx, err, operand_ptr) RISKY_EXPRESSION_SAFE(ASTOptimize_SimpleOptimizations(ctx, (ASTNode**) &operand_ptr), err)

attribute_pure
static bool IsNodeLiteral(const ASTNode *node) {
    return node->type == AST_LITERAL;
}

attribute_const
int64_t fast_pow_int64(const int64_t base, const uint32_t exp) {
    if (exp == 0) {
        return 1;
    }
    if (base == 0) {
        return 0;
    }

    int64_t result = 1;
    int64_t current_base = base;
    uint32_t u_exp = exp;

    while (u_exp > 0) {
        if (u_exp % 2 == 1) {
            if (__builtin_mul_overflow(result, current_base, &result)) {
                return LLONG_MAX;
            }
        }
        if (__builtin_mul_overflow(current_base, current_base, &current_base)) {
            return LLONG_MAX;
        }
        u_exp /= 2;
    }

    return result;
}

attribute_const
static int64_t IntBinaryEval(const int64_t left, const int64_t right, const ASTBinaryType op) {
    switch (op) {
        case AST_BINARY_ADD:
            return left + right;
        case AST_BINARY_SUB:
            return left - right;
        case AST_BINARY_MUL:
            return left * right;
        case AST_BINARY_POW:
            if (right < 0) return 0;
            return fast_pow_int64(left, (uint32_t) right);
        case AST_BINARY_DIV:
        case AST_BINARY_INT_DIV:
            return left / right;
        case AST_BINARY_MOD:
            return left % right;
        case AST_BINARY_LESS_THAN:
            return left < right;
        case AST_BINARY_LESS_THAN_OR_EQUALS:
            return left <= right;
        case AST_BINARY_GREATER_THAN:
            return left > right;
        case AST_BINARY_GREATER_THAN_OR_EQUALS:
            return left >= right;
        case AST_BINARY_EQUALS:
            return left == right;
        case AST_BINARY_NOT_EQUALS:
            return left != right;
        case AST_BINARY_BITWISE_OR:
            return left | right;
        case AST_BINARY_BITWISE_AND:
            return left & right;
        case AST_BINARY_LOGICAL_OR:
            return left || right;
        case AST_BINARY_LOGICAL_AND:
            return left && right;
        case AST_BINARY_SHIFT_LEFT:
            return left << right;
        case AST_BINARY_SHIFT_RIGHT:
            return left >> right;
        default:
            return 0;
    }
}

attribute_const
static double DoubleBinaryEval(const double left, const double right, const ASTBinaryType op) {
    switch (op) {
        case AST_BINARY_ADD:
            return left + right;
        case AST_BINARY_SUB:
            return left - right;
        case AST_BINARY_MUL:
            return left * right;
        case AST_BINARY_POW:
            return pow(left, right);
        case AST_BINARY_DIV:
        case AST_BINARY_INT_DIV:
            return left / right;
        case AST_BINARY_MOD:
            return remainder(left, right);
        case AST_BINARY_LESS_THAN:
            return left < right;
        case AST_BINARY_LESS_THAN_OR_EQUALS:
            return left <= right;
        case AST_BINARY_GREATER_THAN:
            return left > right;
        case AST_BINARY_GREATER_THAN_OR_EQUALS:
            return left >= right;
        case AST_BINARY_EQUALS:
            return left == right;
        case AST_BINARY_NOT_EQUALS:
            return left != right;
        case AST_BINARY_LOGICAL_OR:
            return left || right;
        case AST_BINARY_LOGICAL_AND:
            return left && right;
        default:
            return 0;
    }
}

attribute_const
static int64_t IntUnaryEval(const int64_t operand, const ASTUnaryType op) {
    switch (op) {
        case AST_UNARY_PLUS:
            return operand;
        case AST_UNARY_MINUS:
            return -operand;
        case AST_UNARY_LOGICAL_NOT:
            return !operand;
        case AST_UNARY_BITWISE_NOT:
            return ~operand;
        default:
            return 0;
    }
}

attribute_const
static double FloatUnaryEval(const double operand, const ASTUnaryType op) {
    switch (op) {
        case AST_UNARY_PLUS:
            return operand;
        case AST_UNARY_MINUS:
            return -operand;
        default:
            return 0;
    }
}

static errno_t ConstantUnaryEval(const VValue value, const ASTUnaryType op, VValue *result) {
    switch (value.type) {
        case VALUE_I64: {
            *result = (VValue){
                .type = VALUE_I64,
                .i64 = IntUnaryEval(value.i64, op),
            };
            return VISMUT_ERROR_OK;
        }
        case VALUE_F64:
            if (op == AST_UNARY_LOGICAL_NOT) {
                *result = (VValue){
                    .type = VALUE_I64,
                    .i64 = !value.f64,
                };
            } else {
                *result = (VValue){
                    .type = VALUE_F64,
                    .f64 = FloatUnaryEval(value.f64, op),
                };
            }
            return VISMUT_ERROR_OK;
        default:
            return VISMUT_ERROR_UNSUPPORTED_OPERATION;
    }
}

static errno_t ConstantBinaryEval(const VValue left, const VValue right, const ASTBinaryType op, VValue *result) {
    DEBUG_ASSERT(left.type == right.type);

    switch (left.type) {
        case VALUE_I64: {
            if (unlikely(op == AST_BINARY_DIV)) {
                *result = (VValue){
                    .type = VALUE_F64,
                    .f64 = (double) left.i64 / (double) right.i64,
                };
            } else {
                const int64_t result_value = IntBinaryEval(left.i64, right.i64, op);
                *result = (VValue){
                    .type = VALUE_I64,
                    .i64 = result_value
                };
            }
            return VISMUT_ERROR_OK;
        }
        case VALUE_F64: {
            if (unlikely(op == AST_BINARY_INT_DIV)) {
                *result = (VValue){
                    .type = VALUE_F64,
                    .f64 = floor(left.f64 / right.f64),
                };
            } else {
                const double result_value = DoubleBinaryEval(left.f64, right.f64, op);
                *result = (VValue){
                    .type = VALUE_F64,
                    .f64 = result_value
                };
            }
            return VISMUT_ERROR_OK;
        }
        default:
            return VISMUT_ERROR_UNSUPPORTED_OPERATION;
    }
}

typedef enum {
    ZERO_LITERAL,
    ONE_LITERAL,
    TWO_LITERAL,
    HALF_LITERAL,
    OTHER_LITERAL,
} LiteralValueType;

LiteralValueType GetLiteralValueType(const ASTNode *node) {
    DEBUG_ASSERT(node != NULL);
    DEBUG_ASSERT(node->type == AST_LITERAL);

    if (node->literal.type == VALUE_I64) {
        if (node->literal.i64 == 0) return ZERO_LITERAL;
        if (node->literal.i64 == 1) return ONE_LITERAL;
        if (node->literal.i64 == 2) return TWO_LITERAL;
    } else if (node->literal.type == VALUE_F64) {
        if (node->literal.f64 == 0.0f) return ZERO_LITERAL;
        if (node->literal.f64 == 1.0f) return ONE_LITERAL;
        if (node->literal.f64 == 2.0f) return TWO_LITERAL;
        if (node->literal.f64 == 0.5f) return HALF_LITERAL;
    }

    return OTHER_LITERAL;
}

#define ZERO_VALUE(type_) (((type_) == VALUE_I64) ? (VValue){.type = VALUE_I64, .i64 = 0} : (VValue){.type = VALUE_F64, .f64 = 0.0f})
#define ONE_VALUE(type_) (((type_) == VALUE_I64) ? (VValue){.type = VALUE_I64, .i64 = 1} : (VValue){.type = VALUE_F64, .f64 = 1.0f})

static errno_t ASTOptimize_BinaryExpression(const SimpleOptimizationsContext *ctx, ASTNode **node) {
    DEBUG_ASSERT((*node)->type == AST_BINARY);

    ASTNode *left_operand = (ASTNode *) (*node)->binary_op.left;
    const ASTNode *right_operand = (ASTNode *) (*node)->binary_op.right;
    const ASTBinaryType op = (*node)->binary_op.op;
    const bool is_left_literal = IsNodeLiteral(left_operand);
    const bool is_right_literal = IsNodeLiteral(right_operand);

    if (is_left_literal && is_right_literal) {
        VValue result;
        errno_t err;
        if ((err = ConstantBinaryEval(left_operand->literal, right_operand->literal, (*node)->binary_op.op, &result)) !=
            VISMUT_ERROR_OK) {
            return err;
        }
        *node = CreateLiteralNode(ctx->arena, (*node)->pos, result);
        return VISMUT_ERROR_OK;
    }

    if (is_right_literal) {
        const LiteralValueType literal_value_type = GetLiteralValueType(right_operand);

        if (op == AST_BINARY_MUL) {
            if (literal_value_type == ZERO_LITERAL) {
                *node = CreateLiteralNode(ctx->arena, (*node)->pos, ZERO_VALUE(right_operand->literal.type));
                return VISMUT_ERROR_OK;
            }
            if (literal_value_type == ONE_LITERAL) {
                *node = left_operand;
                return VISMUT_ERROR_OK;
            }
        } else if (op == AST_BINARY_ADD) {
            if (literal_value_type == ZERO_LITERAL) {
                *node = left_operand;
                return VISMUT_ERROR_OK;
            }
        } else if (op == AST_BINARY_POW) {
            if (literal_value_type == ZERO_LITERAL) {
                *node = CreateLiteralNode(ctx->arena, (*node)->pos, ONE_VALUE(left_operand->literal.type));
                return VISMUT_ERROR_OK;
            }
            if (literal_value_type == ONE_LITERAL) {
                *node = left_operand;
                return VISMUT_ERROR_OK;
            }
        }
    } else if (is_left_literal) {
        const LiteralValueType literal_value_type = GetLiteralValueType(left_operand);

        if (op == AST_BINARY_MUL) {
            if (literal_value_type == ZERO_LITERAL) {
                *node = CreateLiteralNode(ctx->arena, (*node)->pos, ZERO_VALUE(right_operand->literal.type));
                return VISMUT_ERROR_OK;
            }
            if (literal_value_type == ONE_LITERAL) {
                *node = left_operand;
                return VISMUT_ERROR_OK;
            }
        } else if (op == AST_BINARY_ADD) {
            if (literal_value_type == ZERO_LITERAL) {
                *node = left_operand;
                return VISMUT_ERROR_OK;
            }
        } else if (op == AST_BINARY_POW) {
            if (literal_value_type == ZERO_LITERAL) {
                *node = CreateLiteralNode(ctx->arena, (*node)->pos, ZERO_VALUE(right_operand->literal.type));
                return VISMUT_ERROR_OK;
            }
            if (literal_value_type == ONE_LITERAL) {
                *node = CreateLiteralNode(ctx->arena, (*node)->pos, ONE_VALUE(right_operand->literal.type));
                return VISMUT_ERROR_OK;
            }
        }
    }

    return VISMUT_ERROR_OK;
}

static errno_t ASTOptimize_UnaryExpression(const SimpleOptimizationsContext *ctx, ASTNode **node) {
    DEBUG_ASSERT((*node)->type == AST_UNARY);

    const ASTNode *operand = (ASTNode *) (*node)->unary_op.operand;
    const ASTUnaryType op = (*node)->unary_op.op;

    if (IsNodeLiteral(operand)) {
        errno_t err;
        VValue result;
        if ((err = ConstantUnaryEval(operand->literal, op, &result)) != VISMUT_ERROR_OK) {
            return err;
        }
        *node = CreateLiteralNode(ctx->arena, (*node)->pos, result);
    }

    return VISMUT_ERROR_OK;
}

VValue ConstantTypeCastEval(const VValue value, const VValueType target_type) {
    switch (value.type) {
        case VALUE_I64:
            if (target_type == VALUE_F64) {
                return (VValue){
                    .type = VALUE_F64,
                    .f64 = (double) value.i64
                };
            }
            return (VValue){0};
        case VALUE_F64:
            if (target_type == VALUE_I64) {
                return (VValue){
                    .type = VALUE_I64,
                    .i64 = (int64_t) value.f64
                };
            }
            return (VValue){0};
        default:
            return (VValue){0};
    }
}

static errno_t ASTOptimize_TypeCast(const SimpleOptimizationsContext *ctx, ASTNode **node) {
    DEBUG_ASSERT((*node)->type == AST_TYPE_CAST);

    const ASTNode *operand = (ASTNode *) (*node)->unary_op.operand;

    if (IsNodeLiteral(operand)) {
        *node = CreateLiteralNode(
            ctx->arena, (*node)->pos,
            ConstantTypeCastEval(operand->literal, (*node)->type_cast.target_type)
        );
    }

    return VISMUT_ERROR_OK;
}

bool LiteralToBoolean(const VValue literal) {
    switch (literal.type) {
        case VALUE_I64:
            return !!literal.i64;
        case VALUE_F64:
            return !!literal.f64;
        default:
            return false;
    }
}

static errno_t ASTOptimize_SimpleOptimizations(SimpleOptimizationsContext *ctx, ASTNode **node) {
    DEBUG_ASSERT(node != NULL && *node != NULL);
    errno_t err;

    switch ((*node)->type) {
        case AST_BINARY: {
            SAFE_SIMPLE_OPTIMIZATIONS(ctx, err, (*node)->binary_op.left);
            SAFE_SIMPLE_OPTIMIZATIONS(ctx, err, (*node)->binary_op.right);
            if (!(*node)->binary_op.is_pure) {
                return VISMUT_ERROR_OK;
            }
            return ASTOptimize_BinaryExpression(ctx, node);
        }
        case AST_UNARY: {
            SAFE_SIMPLE_OPTIMIZATIONS(ctx, err, (*node)->unary_op.operand);
            if (!(*node)->unary_op.is_pure) {
                return VISMUT_ERROR_OK;
            }
            return ASTOptimize_UnaryExpression(ctx, node);
        }
        case AST_TERNARY: {
            const ASTNode *condition = (ASTNode *) (*node)->ternary_op.condition;
            ASTNode *then_expression = (ASTNode *) (*node)->ternary_op.then_expression;
            ASTNode *else_expression = (ASTNode *) (*node)->ternary_op.else_expression;
            SAFE_SIMPLE_OPTIMIZATIONS(ctx, err, (*node)->ternary_op.condition);
            SAFE_SIMPLE_OPTIMIZATIONS(ctx, err, (*node)->ternary_op.then_expression);
            SAFE_SIMPLE_OPTIMIZATIONS(ctx, err, (*node)->ternary_op.else_expression);
            if (IsNodeLiteral(condition)) {
                if (LiteralToBoolean(condition->literal)) {
                    *node = then_expression;
                    return VISMUT_ERROR_OK;
                }
                *node = else_expression;
                return VISMUT_ERROR_OK;
            }
            return VISMUT_ERROR_OK;
        }
        case AST_PRINT_STMT: {
            ASTNode **current = (ASTNode **) &(*node)->print_stmt.expressions;
            while (*current != NULL) {
                ASTNode *next = (ASTNode *) (*current)->next_node;
                RISKY_EXPRESSION_SAFE(ASTOptimize_SimpleOptimizations(ctx, current), err);
                (*current)->next_node = (struct ASTNode *) next;
                current = (ASTNode **) &(*current)->next_node;
            }
            return VISMUT_ERROR_OK;
        }
        case AST_TYPE_CAST: {
            SAFE_SIMPLE_OPTIMIZATIONS(ctx, err, (*node)->type_cast.expression);
            if ((*node)->type_cast.from_type == (*node)->type_cast.target_type) {
                ASTNode *expression = (ASTNode *) (*node)->type_cast.expression;
                expression->pos = (*node)->pos;
                *node = expression;
                return VISMUT_ERROR_OK;
            }
            if (!(*node)->type_cast.is_pure) {
                return VISMUT_ERROR_OK;
            }
            return ASTOptimize_TypeCast(ctx, node);
        }
        case AST_VAR_DECL: {
            ASTNode *initial_value = (ASTNode *) (*node)->var_decl.init_value;
            if (initial_value == NULL) {
                return VISMUT_ERROR_OK;
            }
            SAFE_SIMPLE_OPTIMIZATIONS(ctx, err, (*node)->var_decl.init_value);
            return VISMUT_ERROR_OK;
        }
        case AST_MODULE: {
            ASTNode **current = (ASTNode **) &(*node)->module.statements;
            while (*current != NULL) {
                ASTNode *next = (ASTNode *) (*current)->next_node;
                RISKY_EXPRESSION_SAFE(ASTOptimize_SimpleOptimizations(ctx, current), err);
                (*current)->next_node = (struct ASTNode *) next;
                current = (ASTNode **) &(*current)->next_node;
            }
            return VISMUT_ERROR_OK;
        }
        default:
            return VISMUT_ERROR_OK;
    }
}

errno_t ASTOptimize(Arena *arena, ASTNode *node) {
    SimpleOptimizationsContext ctx = {
        .arena = arena,
    };

    errno_t err;
    if ((err = ASTOptimize_SimpleOptimizations(&ctx, &node))) {
        return err;
    }

    return VISMUT_ERROR_OK;
}
