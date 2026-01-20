#include "ast_analyze.h"

#include "ast_typing.h"
#include "../types.h"
#include "../errors/errors.h"
#include "ast.h"

typedef struct {
    Scope *current_scope;
    Arena *arena;
} ASTTypeAnalyzerContext;

#define SAFE_ANALYZE(node_ptr, out_type_ptr) \
    do { \
        if (unlikely((err = ASTTypeAnalyzeNode(context, (ASTNode *)node_ptr, out_type_ptr)) != VISMUT_ERROR_OK)) { \
            return err;\
        }\
    } while (0)

attribute_pure
static bool IsNodePure(const ASTNode *node) {
    switch (node->type) {
        case AST_BINARY:
            return node->binary_op.is_pure;
        case AST_UNARY:
            return node->unary_op.is_pure;
        case AST_TYPE_CAST:
            return node->type_cast.is_pure;
        case AST_FUNCTION_CALL:
            return false
        default:
            return true;
    }
}

static errno_t ASTTypeAnalyzeNode(ASTTypeAnalyzerContext *context, ASTNode *node, VValueType *value_type) {
    DEBUG_ASSERT(node != NULL);

    errno_t err;
    switch (node->type) {
        case AST_BINARY: {
            VValueType left, right;
            SAFE_ANALYZE(node->binary_op.left, &left);
            SAFE_ANALYZE(node->binary_op.right, &right);

            const bool operands_is_pure = IsNodePure((const ASTNode *) node->binary_op.right) &&
                                          IsNodePure((const ASTNode *) node->binary_op.left);
            node->binary_op.is_pure = operands_is_pure && node->binary_op.is_pure;

            if (node->binary_op.op == AST_BINARY_ASSIGN) {
                const bool is_allowed_cast = IsCastAllowed(right, left, false);
                if (!is_allowed_cast) {
                    return VISMUT_ERROR_CAST_IS_NOT_ALLOWED;
                }
                ASTNode *right_node = (ASTNode *) node->binary_op.right;
                ASTNode *cast_node = CreateTypeCastNode(context->arena, right_node->pos, right_node, left);
                cast_node->type_cast.from_type = right;
                node->binary_op.right = (struct ASTNode *) cast_node;
                *value_type = node->binary_op.expr_type = left;
                return VISMUT_ERROR_OK;
            }

            const VValueType result = GetBinaryOpResultType(node->binary_op.op, left, right);
            if (likely(result != VALUE_UNKNOWN)) {
                *value_type = node->binary_op.expr_type = result;
                return VISMUT_ERROR_OK;
            }

            const VValueType common_type = FindCommonType(left, right);
            if (unlikely(common_type == VALUE_UNKNOWN)) {
                wprintf(L"Error in node:\n");
                ASTNode_Print(node);
                wprintf(L"Operation: '<%ls> %ls <%ls>' is unsupported\n", VValueType_String(left),
                        ASTBinaryType_String(node->binary_op.op), VValueType_String(right));
                return VISMUT_ERROR_UNSUPPORTED_OPERATION;
            }

            if (common_type == left) {
                // casting right operand
                ASTNode *right_node = (ASTNode *) node->binary_op.right;
                ASTNode *cast_node = CreateTypeCastNode(context->arena, right_node->pos, right_node, common_type);
                cast_node->type_cast.from_type = right;
                node->binary_op.right = (struct ASTNode *) cast_node;
            } else {
                // casting left operand
                ASTNode *left_node = (ASTNode *) node->binary_op.left;
                ASTNode *cast_node = CreateTypeCastNode(context->arena, left_node->pos, left_node, common_type);
                cast_node->type_cast.from_type = left;
                node->binary_op.left = (struct ASTNode *) cast_node;
            }
            const VValueType result_with_casting = GetBinaryOpResultType(node->binary_op.op, common_type, common_type);
            if (unlikely(result_with_casting == VALUE_UNKNOWN)) {
                wprintf(L"Error in node:\n");
                ASTNode_Print(node);
                wprintf(L"Operation: '<%ls> %ls <%ls>' is unsupported\n", VValueType_String(common_type),
                        ASTBinaryType_String(node->binary_op.op), VValueType_String(common_type));
                return VISMUT_ERROR_UNSUPPORTED_OPERATION;
            }

            *value_type = node->binary_op.expr_type = result_with_casting;
            return VISMUT_ERROR_OK;
        }
        case AST_UNARY: {
            VValueType operand;
            SAFE_ANALYZE(node->unary_op.operand, &operand);

            const bool operand_is_pure = IsNodePure((const ASTNode *) node->unary_op.operand);
            node->unary_op.is_pure = operand_is_pure && node->unary_op.is_pure;

            const VValueType result = GetUnaryOpResultType(node->unary_op.op, operand);
            if (result == VALUE_UNKNOWN) {
                wprintf(L"Error in node:\n");
                ASTNode_Print(node);
                wprintf(L"'<%ls>' for '<%ls>' is unsupported\n", ASTUnaryType_String(node->unary_op.op),
                        VValueType_String(operand));
                return VISMUT_ERROR_UNSUPPORTED_OPERATION;
            }

            *value_type = node->unary_op.expr_type = result;
            return VISMUT_ERROR_OK;
        }
        case AST_TERNARY: {
            VValueType condition, then_expression, else_expression;
            SAFE_ANALYZE(node->ternary_op.condition, &condition);
            SAFE_ANALYZE(node->ternary_op.then_expression, &then_expression);
            SAFE_ANALYZE(node->ternary_op.else_expression, &else_expression);

            node->ternary_op.is_pure = IsNodePure((ASTNode *) node->ternary_op.then_expression)
                                       && IsNodePure((ASTNode *) node->ternary_op.else_expression);

            if (likely(then_expression == else_expression)) {
                *value_type = node->ternary_op.expr_type = then_expression;
                return VISMUT_ERROR_OK;
            }

            const VValueType common_type = FindCommonType(then_expression, else_expression);
            if (unlikely(common_type == VALUE_UNKNOWN)) {
                return VISMUT_ERROR_CAST_IS_NOT_ALLOWED;
            }

            if (common_type == then_expression) {
                // casting else expression
                ASTNode *else_expression_node = (ASTNode *) node->ternary_op.else_expression;
                ASTNode *cast_node = CreateTypeCastNode(context->arena, else_expression_node->pos, else_expression_node,
                                                        common_type);
                cast_node->type_cast.from_type = else_expression;
                node->ternary_op.else_expression = (struct ASTNode *) cast_node;
            } else {
                // casting then expression
                ASTNode *then_expression_node = (ASTNode *) node->ternary_op.then_expression;
                ASTNode *cast_node = CreateTypeCastNode(context->arena, then_expression_node->pos, then_expression_node,
                                                        common_type);
                cast_node->type_cast.from_type = then_expression;
                node->ternary_op.then_expression = (struct ASTNode *) cast_node;
            }
            *value_type = node->ternary_op.expr_type = common_type;
            return VISMUT_ERROR_OK;
        }
        case AST_PRINT_STMT: {
            for (const ASTNode *current = (ASTNode *) node->print_stmt.expressions; current != NULL;
                 current = (ASTNode *) current->next_node) {
                VValueType type;
                SAFE_ANALYZE(current, &type);
            }
            return VISMUT_ERROR_OK;
        }
        case AST_VAR_REF: {
            const Symbol *var_symbol = Scope_Resolve(context->current_scope, node->var_ref.var_name);
            if (var_symbol == NULL) {
                return VISMUT_ERROR_SYMBOL_NOT_DEFINED;
            }
            Scope_MarkUsed(context->current_scope, node->var_ref.var_name);
            *value_type = node->var_ref.expr_type = var_symbol->value.type;
            return VISMUT_ERROR_OK;
        }
        case AST_VAR_DECL: {
            *value_type = VALUE_VOID;
            VValueType init_value;
            if (!node->var_decl.init_value) {
                init_value = node->var_decl.var_type;
            } else {
                SAFE_ANALYZE(node->var_decl.init_value, &init_value);
                node->var_decl.init_value_type = init_value;
            }

            if (node->var_decl.var_type == VALUE_AUTO) {
                node->var_decl.var_type = init_value;
            } else if (node->var_decl.var_type != init_value) {
                wprintf(L"Error in node:");
                ASTNode_Print(node);
                wprintf(L"Type %ls != %ls\n", VValueType_String(node->var_decl.var_type),
                        VValueType_String(init_value));
                return VISMUT_ERROR_TYPE_IS_INCOMPATIBLE;
            }

            if ((err = Scope_Declare(context->current_scope, node->var_decl.var_name, init_value, 0)) !=
                VISMUT_ERROR_OK) {
                return err;
            }

            return VISMUT_ERROR_OK;
        }
        case AST_IF_STMT: {
            *value_type = VALUE_VOID;
            VValueType condition, then_block, else_block;
            SAFE_ANALYZE(node->if_stmt.condition, &condition);
            SAFE_ANALYZE(node->if_stmt.then_block, &then_block);
            if (node->if_stmt.else_block != NULL) {
                SAFE_ANALYZE(node->if_stmt.else_block, &else_block);
            }
            return VISMUT_ERROR_OK;
        }
        case AST_BLOCK: {
            *value_type = VALUE_VOID;
            context->current_scope = node->block.scope;

            ASTNode *current_statement = (ASTNode *) node->block.statements;
            while (current_statement != NULL) {
                VValueType statement_type;
                SAFE_ANALYZE(current_statement, &statement_type);
                current_statement = (ASTNode *) current_statement->next_node;
            }

            Scope_RemoveUnused(context->current_scope);

            context->current_scope = context->current_scope->parent;
            return VISMUT_ERROR_OK;
        }
        case AST_MODULE: {
            *value_type = VALUE_VOID;
            ASTNode *current_statement = (ASTNode *) node->module.statements;
            while (current_statement != NULL) {
                VValueType statement_type;
                SAFE_ANALYZE(current_statement, &statement_type);
                current_statement = (ASTNode *) current_statement->next_node;
            }
            return VISMUT_ERROR_OK;
        }
        case AST_LITERAL: {
            *value_type = node->literal.type;
            return VISMUT_ERROR_OK;
        }
        case AST_TYPE_CAST: {
            VValueType expression;
            SAFE_ANALYZE(node->type_cast.expression, &expression);

            const bool casting_expression_is_pure = IsNodePure((const ASTNode *) node->type_cast.expression);
            node->type_cast.is_pure = casting_expression_is_pure;

            node->type_cast.from_type = expression;
            if (!IsCastAllowed(expression, node->type_cast.target_type, node->type_cast.is_explicit)) {
                return VISMUT_ERROR_CAST_IS_NOT_ALLOWED;
            }

            *value_type = node->type_cast.target_type;
            return VISMUT_ERROR_OK;
        }
        default:
            return VISMUT_ERROR_OK;
    }
}

errno_t ASTModuleTypeAnalyze(Arena *arena, ASTNode *node) {
    DEBUG_ASSERT(node->type == AST_MODULE);

    ASTTypeAnalyzerContext ctx = {
        .current_scope = node->module.scope,
        .arena = arena,
    };
    VValueType type;

    return ASTTypeAnalyzeNode(&ctx, node, &type);
}
