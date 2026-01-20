#include "ast.h"

#include <stdio.h>

#include "../ansi_colors.h"

static void ASTNode_PrintNode(const ASTNode *, int);

void ASTNode_Print(const ASTNode *node) {
    ASTNode_PrintNode(node, 0);
}

static void print_value(const VValue *value) {
    if (!value) {
        printf("<NULL>");
        return;
    }

    switch (value->type) {
        case VALUE_VOID:
            ansi_set_color(ANSI_COLOR_MAGENTA);
            printf("void");
            break;
        case VALUE_AUTO:
            ansi_set_color(ANSI_COLOR_MAGENTA);
            printf("auto");
            break;
        case VALUE_I64:
            ansi_set_color(ANSI_COLOR_BRIGHT_BLUE);
            printf("%lld", value->i64);
            break;
        case VALUE_F64:
            ansi_set_color(ANSI_COLOR_BRIGHT_BLUE);
            printf("%f", value->f64);
            break;
        case VALUE_STR:
            ansi_set_color(ANSI_COLOR_BRIGHT_GREEN);
            printf("\"%s\"", (char *) value->str);
            break;
        default:
            printf("<unknown value type>");
    }
}

static void print_indent(const int ident) {
    for (int i = 0; i < ident; ++i) {
        if (i % 2 == 0) {
            PRINT_FG(ANSI_COLOR_WHITE, "|   ");
        } else {
            PRINT_STYLED(ANSI_COLOR_WHITE, ANSI_BG_BLACK, ANSI_UNDERLINE, "|");
            PRINT_FG(ANSI_COLOR_WHITE, "   ");
        }
    }
}

#define PRINT_EXPR_TYPE(EXPR_TYPE) \
    do { \
        printf(" (");\
        PRINT_FG(ANSI_COLOR_CYAN, VValueType_String(EXPR_TYPE));\
        printf(")");\
    }\
    while (0)

#define PRINT_PURE(IS_PURE) \
    do { \
        if (IS_PURE) { \
            PRINT_FG(ANSI_COLOR_GREEN, " pure"); \
        } else {\
            PRINT_FG(ANSI_COLOR_RED, " !pure"); \
        }\
    } while (0)

#define PRINT_NODE_POS(node_ptr) \
    do {\
        ansi_set_color(ANSI_COLOR_WHITE); \
        printf(" ["); \
        ansi_set_color(ANSI_COLOR_BRIGHT_RED); \
        printf("%zu", (node_ptr)->pos.offset); \
        ansi_set_color(ANSI_COLOR_WHITE);\
        printf("-"); \
        ansi_set_color(ANSI_COLOR_BRIGHT_RED); \
        printf("%zu", (node_ptr)->pos.offset + (node_ptr)->pos.length); \
        ansi_set_color(ANSI_COLOR_WHITE); \
        printf("]\n");\
    } while (0)

static void ASTNode_PrintNode(const ASTNode *node, const int depth) {
    if (!node) {
        print_indent(depth);
        printf("<NULL>\n");
        return;
    }

    print_indent(depth);

    PRINT_FG(ANSI_COLOR_BRIGHT_MAGENTA, ASTNodeType_String(node->type));
    switch (node->type) {
        case AST_LITERAL:
            printf(" ");
            print_value(&node->literal);
            ansi_reset();
            PRINT_EXPR_TYPE(node->literal.type);
            PRINT_NODE_POS(node);
            break;

        case AST_VAR_REF:
            printf(" ");
            PRINT_FG(ANSI_COLOR_BRIGHT_YELLOW,
                     node->var_ref.var_name ? (const char*) node->var_ref.var_name : "<NULL>");
            PRINT_EXPR_TYPE(node->var_ref.expr_type);
            PRINT_NODE_POS(node);
            break;

        case AST_BINARY:
            printf(" ");
            PRINT_FG(ANSI_COLOR_GREEN, ASTBinaryType_String(node->binary_op.op));
            PRINT_EXPR_TYPE(node->binary_op.expr_type);
            PRINT_PURE(node->binary_op.is_pure);
            PRINT_NODE_POS(node);
            print_indent(depth + 1);
            PRINT_FG(ANSI_COLOR_YELLOW, "left\n");
            ASTNode_PrintNode((const ASTNode *) node->binary_op.left, depth + 2);
            print_indent(depth + 1);
            PRINT_FG(ANSI_COLOR_YELLOW, "right\n");
            ASTNode_PrintNode((const ASTNode *) node->binary_op.right, depth + 2);
            break;

        case AST_UNARY:
            printf(" ");
            PRINT_FG(ANSI_COLOR_GREEN, ASTUnaryType_String(node->unary_op.op));
            PRINT_EXPR_TYPE(node->unary_op.expr_type);
            PRINT_PURE(node->unary_op.is_pure);
            PRINT_NODE_POS(node);
            ASTNode_PrintNode((const ASTNode *) node->unary_op.operand, depth + 1);
            break;

        case AST_TERNARY:
            PRINT_EXPR_TYPE(node->ternary_op.expr_type);
            PRINT_PURE(node->ternary_op.is_pure);
            PRINT_NODE_POS(node);
            ASTNode_PrintNode((const ASTNode *) node->ternary_op.condition, depth + 1);

            print_indent(depth + 1);
            PRINT_FG(ANSI_COLOR_YELLOW, "then\n");
            ASTNode_PrintNode((const ASTNode *) node->ternary_op.then_expression, depth + 2);

            print_indent(depth + 1);
            PRINT_FG(ANSI_COLOR_YELLOW, "else\n");
            ASTNode_PrintNode((const ASTNode *) node->ternary_op.else_expression, depth + 2);
            break;
        case AST_VAR_DECL:
            printf(" ");
            PRINT_FG(ANSI_COLOR_BRIGHT_YELLOW,
                     node->var_decl.var_name ? (const char*)node->var_decl.var_name : "<NULL>");
            PRINT_EXPR_TYPE(node->var_decl.var_type);
            PRINT_NODE_POS(node);
            if (node->var_decl.init_value) {
                print_indent(depth + 1);
                PRINT_FG(ANSI_COLOR_YELLOW, "init");
                PRINT_EXPR_TYPE(node->var_decl.init_value_type);
                printf("\n");
                ASTNode_PrintNode((const ASTNode *) node->var_decl.init_value, depth + 2);
            }
            break;

        case AST_BLOCK:
            PRINT_NODE_POS(node);
            if (node->block.statements) {
                const ASTNode *stmt = (const ASTNode *) node->module.statements;
                while (stmt) {
                    ASTNode_PrintNode(stmt, depth + 1);
                    stmt = (const ASTNode *) stmt->next_node;
                }
            }
            break;

        case AST_IF_STMT:
            PRINT_NODE_POS(node);
            ASTNode_PrintNode((const ASTNode *) node->if_stmt.condition, depth + 1);
            print_indent(depth + 1);
            PRINT_FG(ANSI_COLOR_YELLOW, "then\n");
            ASTNode_PrintNode((const ASTNode *) node->if_stmt.then_block, depth + 2);
            if (node->if_stmt.else_block) {
                print_indent(depth + 1);
                PRINT_FG(ANSI_COLOR_YELLOW, "else\n");
                ASTNode_PrintNode((const ASTNode *) node->if_stmt.else_block, depth + 2);
            }
            break;

        case AST_WHILE_STMT:
            PRINT_NODE_POS(node);
            print_indent(depth + 1);
            printf("condition:\n");
            ASTNode_PrintNode((const ASTNode *) node->while_stmt.condition, depth + 2);
            print_indent(depth + 1);
            printf("body:\n");
            ASTNode_PrintNode((const ASTNode *) node->while_stmt.body, depth + 2);
            break;

        case AST_TYPE_CAST:
            PRINT_EXPR_TYPE(node->type_cast.from_type);
            PRINT_FG(ANSI_COLOR_YELLOW, " ->");
            PRINT_EXPR_TYPE(node->type_cast.target_type);
            PRINT_PURE(node->type_cast.is_pure);
            PRINT_NODE_POS(node);
            ASTNode_PrintNode((const ASTNode *) node->type_cast.expression, depth + 1);
            break;

        case AST_PRINT_STMT:
            PRINT_NODE_POS(node);
            for (const ASTNode *current = (ASTNode *) node->print_stmt.expressions; current != NULL;
                 current = (ASTNode *) current->next_node) {
                ASTNode_PrintNode(current, depth + 1);
            }
            break;

        case AST_MODULE:
            printf(" ");
            ansi_set_color(ANSI_COLOR_BLACK);
            ansi_set_bg_color(ANSI_BG_WHITE);
            printf("%s", node->module.module_name ? (char *) node->module.module_name : "<unnamed>");
            ansi_reset();
            PRINT_NODE_POS(node);

            if (node->module.functions) {
                print_indent(depth + 1);
                ansi_print_styled(ANSI_COLOR_WHITE, ANSI_BG_BLACK, ANSI_UNDERLINE, "functions");
                printf("\n");
                for (const ASTNode *current = (ASTNode *) node->module.functions; current != NULL;
                     current = (ASTNode *) current->next_node) {
                    ASTNode_PrintNode(current, depth + 2);
                }
            }
            if (node->module.statements) {
                print_indent(depth + 1);
                ansi_print_styled(ANSI_COLOR_WHITE, ANSI_BG_BLACK, ANSI_UNDERLINE, "statements");
                printf("\n");
                for (const ASTNode *current = (ASTNode *) node->module.statements; current != NULL;
                     current = (ASTNode *) current->next_node) {
                    ASTNode_PrintNode(current, depth + 2);
                }
            }
            break;
        case AST_UNKNOWN:
            printf(" <unknown node>");
            break;
        case AST_FUNCTION_DECL:
            printf(" <function decl>");
            break;
        case AST_FUNCTION_CALL:
            printf(" <function call>");
            break;
        case AST_COUNT:
        default:
            printf(" <unhandled node type>\n");
            break;
    }
}


ASTNode *CreateLiteralNode(Arena *arena, const Position pos, const VValue value) {
    ASTNode *node = Arena_Type(arena, ASTNode);
    *node = (ASTNode){
        .type = AST_LITERAL,
        .pos = pos,
        .next_node = NULL,
        .literal = value,
    };
    return node;
}

ASTNode *CreateVarRefNode(Arena *arena, const Position pos, const uint8_t *var_name) {
    ASTNode *node = Arena_Type(arena, ASTNode);
    *node = (ASTNode){
        .type = AST_VAR_REF,
        .pos = pos,
        .next_node = NULL,
        .var_ref = {
            .var_name = var_name,
            .expr_type = VALUE_AUTO,
        },
    };
    return node;
}

ASTNode *CreateBinaryNode(Arena *arena, const Position pos, const ASTNode *left, const ASTNode *right,
                          const ASTBinaryType op, const bool is_pure) {
    ASTNode *node = Arena_Type(arena, ASTNode);
    *node = (ASTNode){
        .type = AST_BINARY,
        .pos = pos,
        .next_node = NULL,
        .binary_op = {
            .left = (struct ASTNode *) left,
            .right = (struct ASTNode *) right,
            .expr_type = VALUE_AUTO,
            .op = op,
            .is_pure = is_pure,
        },
    };
    return node;
}

ASTNode *CreateUnaryNode(Arena *arena, const Position pos, const ASTNode *operand, const ASTUnaryType op,
                         const bool is_pure) {
    ASTNode *node = Arena_Type(arena, ASTNode);
    *node = (ASTNode){
        .type = AST_UNARY,
        .pos = pos,
        .next_node = NULL,
        .unary_op = {
            .operand = (struct ASTNode *) operand,
            .expr_type = VALUE_AUTO,
            .op = op,
            .is_pure = is_pure,
        },
    };
    return node;
}

ASTNode *CreateIfStatementNode(Arena *arena, const Position pos, const ASTNode *condition,
                               const ASTNode *then_block, const ASTNode *else_block) {
    ASTNode *node = Arena_Type(arena, ASTNode);
    *node = (ASTNode){
        .type = AST_IF_STMT,
        .pos = pos,
        .next_node = NULL,
        .if_stmt = {
            .condition = (struct ASTNode *) condition,
            .then_block = (struct ASTNode *) then_block,
            .else_block = (struct ASTNode *) else_block,
        },
    };
    return node;
}

ASTNode *CreateVarDeclarationNode(Arena *arena, const Position pos, const uint8_t *var_name,
                                  const VValueType var_type, const ASTNode *init_value) {
    ASTNode *node = Arena_Type(arena, ASTNode);
    *node = (ASTNode){
        .type = AST_VAR_DECL,
        .pos = pos,
        .next_node = NULL,
        .var_decl = {
            .var_name = var_name,
            .var_type = var_type,
            .init_value_type = VALUE_AUTO,
            .init_value = (struct ASTNode *) init_value,
        },
    };
    return node;
}

ASTNode *CreateTypeCastNode(Arena *arena, const Position pos, ASTNode *expression, const VValueType target_type
                            /*, const bool is_explicit*/) {
    ASTNode *node = Arena_Type(arena, ASTNode);
    *node = (ASTNode){
        .type = AST_TYPE_CAST,
        .pos = pos,
        .next_node = NULL,
        .type_cast = {
            .expression = (struct ASTNode *) expression,
            .target_type = target_type,
            .from_type = VALUE_AUTO,
            .is_explicit = true,
            .is_pure = true,
        },
    };
    return node;
}

ASTNode *CreateTernaryNode(Arena *arena, const Position pos, const ASTNode *condition,
                           const ASTNode *then_expression, const ASTNode *else_expression) {
    ASTNode *node = Arena_Type(arena, ASTNode);
    *node = (ASTNode){
        .type = AST_TERNARY,
        .pos = pos,
        .next_node = NULL,
        .ternary_op = {
            .condition = (struct ASTNode *) condition,
            .then_expression = (struct ASTNode *) then_expression,
            .else_expression = (struct ASTNode *) else_expression,
            .expr_type = VALUE_AUTO,
            .is_pure = true,
        },
    };
    return node;
}

ASTNode *CreateModuleNode(Arena *arena, const uint8_t *module_name, Scope *scope) {
    ASTNode *node = Arena_Type(arena, ASTNode);
    *node = (ASTNode){
        .type = AST_MODULE,
        .pos = (Position){0},
        .next_node = NULL,
        .module = {
            .statements = NULL,
            .functions = NULL,
            .module_name = module_name,
            .scope = scope,
        },
    };
    return node;
}

ASTNode *CreateBlockNode(Arena *arena, const Position pos, ASTNode *statements, Scope *scope) {
    ASTNode *node = Arena_Type(arena, ASTNode);
    *node = (ASTNode){
        .type = AST_BLOCK,
        .pos = pos,
        .next_node = NULL,
        .block = {
            .statements = (struct ASTNode *) statements,
            .scope = scope,
        },
    };
    return node;
}

ASTNode *CreatePrintStatementNode(Arena *arena, const Position pos, ASTNode *expressions) {
    ASTNode *node = Arena_Type(arena, ASTNode);
    *node = (ASTNode){
        .type = AST_PRINT_STMT,
        .pos = pos,
        .next_node = NULL,
        .print_stmt = {
            .expressions = (struct ASTNode *) expressions,
        },
    };
    return node;
}
