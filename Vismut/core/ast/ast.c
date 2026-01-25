#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ansi_colors.h"
#include "../hash/murmur3.h"

static void ASTNode_PrintNode(const ASTNode *, int, FILE *);

void ASTNode_Print(const ASTNode *node, FILE *file) {
    ASTNode_PrintNode(node, 0, file);
}

static void print_value(FILE *file, const VValue *value) {
    if (!value) {
        fprintf(file, "<NULL>");
        return;
    }

    switch (value->type) {
        case VALUE_VOID:
            FPRINT_COLOR(file, ANSI_BRIGHT_MAGENTA_FG, "void");
            break;
        case VALUE_AUTO:
            FPRINT_COLOR(file, ANSI_BRIGHT_MAGENTA_FG, "auto");
            break;
        case VALUE_I64:
            FPRINTF_COLOR(file, ANSI_BRIGHT_BLUE_FG, "%lld", value->i64);
            break;
        case VALUE_F64:
            FPRINTF_COLOR(file, ANSI_BRIGHT_BLUE_FG, "%f", value->f64);
            break;
        case VALUE_STR: {
            SET_FILE_COLOR(file, ANSI_GREEN_FG);
            putc('"', file);
            const uint8_t *ptr = value->str;
            const uint8_t *end = value->str + strlen((const char *) value->str);
            while (ptr != end) {
                const uint8_t c = *ptr;
                switch (c) {
                    case '\n':
                        FPRINT_COLOR(file, ANSI_BLUE_FG, "\\n");
                        SET_FILE_COLOR(file, ANSI_GREEN_FG);
                        break;
                    case '\r':
                        FPRINT_COLOR(file, ANSI_BLUE_FG, "\\r");
                        SET_FILE_COLOR(file, ANSI_GREEN_FG);
                        break;
                    case '\t':
                        FPRINT_COLOR(file, ANSI_BLUE_FG, "\\t");
                        SET_FILE_COLOR(file, ANSI_GREEN_FG);
                        break;
                    case '\b':
                        FPRINT_COLOR(file, ANSI_BLUE_FG, "\\b");
                        SET_FILE_COLOR(file, ANSI_GREEN_FG);
                        break;
                    case '\v':
                        FPRINT_COLOR(file, ANSI_BLUE_FG, "\\v");
                        SET_FILE_COLOR(file, ANSI_GREEN_FG);
                        break;
                    default:
                        putc(c, file);
                        break;
                }
                ++ptr;
            }
            putc('"', file);
            RESET_FILE_COLOR(file);
            break;
        }
        default:
            fprintf(file, "<unknown value type>");
    }
}

static void print_indent(FILE *file, const int ident) {
    for (int i = 0; i < ident; ++i) {
        if (i % 2 == 0) {
            FPRINT_COLOR(file, ANSI_WHITE_FG, "|   ");
        } else {
            FPRINT_3COLOR(file, ANSI_WHITE_FG, ANSI_BLACK_BG, ANSI_UNDERLINE, "|");
            FPRINT_COLOR(file, ANSI_WHITE_FG, "   ");
        }
    }
}

#define PRINT_EXPR_TYPE(FILE, EXPR_TYPE) \
    START_BLOCK_WRAPPER \
        FPRINT_COLOR(FILE, ANSI_BRIGHT_BLACK_FG, " (");\
        FPRINT_COLOR(FILE, ANSI_CYAN_FG, VValueType_String(EXPR_TYPE));\
        FPRINT_COLOR(FILE, ANSI_BRIGHT_BLACK_FG, ")");\
    END_BLOCK_WRAPPER

#define PRINT_PURE(FILE, IS_PURE) \
    START_BLOCK_WRAPPER \
        if (IS_PURE) { \
            FPRINT_COLOR(FILE, ANSI_GREEN_FG, " pure"); \
        } else { \
            FPRINT_COLOR(FILE, ANSI_RED_FG, " !pure"); \
        } \
    END_BLOCK_WRAPPER

#define PRINT_NODE_POS(FILE, node_ptr) \
    START_BLOCK_WRAPPER \
        FPRINTF_COLOR(FILE, ANSI_BRIGHT_BLACK_FG, " [%zu-%zu]\n", (node_ptr)->pos.offset, (node_ptr)->pos.offset + (node_ptr)->pos.length); \
    END_BLOCK_WRAPPER

#define PRINT_LABEL(file, LABEL) \
    FPRINT_COLOR(file, ANSI_YELLOW_FG, LABEL);

#define PRINT_FUNCTION_PARAM(FILE, PARAMS_PTR) \
    START_BLOCK_WRAPPER \
        FPRINT_COLOR(FILE, ANSI_YELLOW_FG, (const char*) (PARAMS_PTR).param_names[i]); \
        FPRINT_COLOR(FILE, ANSI_BRIGHT_BLACK_FG, ": "); \
        FPRINT_COLOR(FILE, ANSI_BLUE_FG, VValueType_String((PARAMS_PTR).param_types[i])); \
    END_BLOCK_WRAPPER

static void ASTNode_PrintNode(const ASTNode *node, const int depth, FILE *file) {
    if (!node) {
        print_indent(file, depth);
        fprintf(file, "<NULL>\n");
        return;
    }

    print_indent(file, depth);


    FPRINT_COLOR(file, ANSI_BRIGHT_MAGENTA_FG, ASTNodeType_String(node->type));
    switch (node->type) {
        case AST_LITERAL:
            putc(' ', file);
            print_value(file, &node->literal);
            PRINT_EXPR_TYPE(file, node->literal.type);
            PRINT_NODE_POS(file, node);
            break;

        case AST_VAR_REF:
            putc(' ', file);
            FPRINT_COLOR(file, ANSI_BRIGHT_YELLOW_FG,
                         node->var_ref.var_name ? (const char *) node->var_ref.var_name : "<NULL>");
            PRINT_EXPR_TYPE(file, node->var_ref.expr_type);
            PRINT_NODE_POS(file, node);
            break;

        case AST_BINARY:
            putc(' ', file);
            FPRINT_COLOR(file, ANSI_BRIGHT_GREEN_FG, ASTBinaryType_String(node->binary_op.op));
            PRINT_EXPR_TYPE(file, node->binary_op.expr_type);
            PRINT_PURE(file, node->binary_op.is_pure);
            PRINT_NODE_POS(file, node);
            print_indent(file, depth + 1);
            PRINT_LABEL(file, "left\n");
            ASTNode_PrintNode((const ASTNode *) node->binary_op.left, depth + 2, file);
            print_indent(file, depth + 1);
            PRINT_LABEL(file, "right\n");
            ASTNode_PrintNode((const ASTNode *) node->binary_op.right, depth + 2, file);
            break;

        case AST_UNARY:
            putc(' ', file);
            FPRINT_COLOR(file, ANSI_BRIGHT_GREEN_FG, ASTUnaryType_String(node->unary_op.op));
            PRINT_EXPR_TYPE(file, node->unary_op.expr_type);
            PRINT_PURE(file, node->unary_op.is_pure);
            PRINT_NODE_POS(file, node);
            ASTNode_PrintNode((const ASTNode *) node->unary_op.operand, depth + 1, file);
            break;

        case AST_TERNARY:
            PRINT_EXPR_TYPE(file, node->ternary_op.expr_type);
            PRINT_PURE(file, node->ternary_op.is_pure);
            PRINT_NODE_POS(file, node);
            ASTNode_PrintNode((const ASTNode *) node->ternary_op.condition, depth + 1, file);

            print_indent(file, depth + 1);
            PRINT_LABEL(file, "then\n");
            ASTNode_PrintNode((const ASTNode *) node->ternary_op.then_expression, depth + 2, file);

            print_indent(file, depth + 1);
            PRINT_LABEL(file, "else\n");
            ASTNode_PrintNode((const ASTNode *) node->ternary_op.else_expression, depth + 2, file);
            break;

        case AST_VAR_DECL:
            putc(' ', file);
            FPRINT_COLOR(file, ANSI_BRIGHT_YELLOW_FG,
                         node->var_decl.var_name ? (const char *) node->var_decl.var_name : "<NULL>");
            PRINT_EXPR_TYPE(file, node->var_decl.var_type);
            PRINT_NODE_POS(file, node);
            if (node->var_decl.init_value) {
                print_indent(file, depth + 1);
                PRINT_LABEL(file, "init");
                PRINT_EXPR_TYPE(file, node->var_decl.init_value_type);
                fprintf(file, "\n");
                ASTNode_PrintNode((const ASTNode *) node->var_decl.init_value, depth + 2, file);
            }
            break;

        case AST_BLOCK:
            PRINT_NODE_POS(file, node);
            if (node->block.statements) {
                const ASTNode *stmt = (const ASTNode *) node->module.statements;
                while (stmt) {
                    ASTNode_PrintNode(stmt, depth + 1, file);
                    stmt = (const ASTNode *) stmt->next_node;
                }
            }
            break;

        case AST_IF_STMT:
            PRINT_NODE_POS(file, node);
            ASTNode_PrintNode((const ASTNode *) node->if_stmt.condition, depth + 1, file);
            print_indent(file, depth + 1);
            PRINT_LABEL(file, "then\n");
            ASTNode_PrintNode((const ASTNode *) node->if_stmt.then_block, depth + 2, file);
            if (node->if_stmt.else_block) {
                print_indent(file, depth + 1);
                PRINT_LABEL(file, "else\n");
                ASTNode_PrintNode((const ASTNode *) node->if_stmt.else_block, depth + 2, file);
            }
            break;

        case AST_WHILE_STMT:
            PRINT_NODE_POS(file, node);
            ASTNode_PrintNode((const ASTNode *) node->while_stmt.condition, depth + 1, file);
            print_indent(file, depth + 1);
            PRINT_LABEL(file, "body\n");
            ASTNode_PrintNode((const ASTNode *) node->while_stmt.body, depth + 2, file);
            break;

        case AST_TYPE_CAST:
            PRINT_EXPR_TYPE(file, node->type_cast.from_type);
            FPRINT_COLOR(file, ANSI_BRIGHT_YELLOW_FG, " ->");
            PRINT_EXPR_TYPE(file, node->type_cast.target_type);
            PRINT_PURE(file, node->type_cast.is_pure);
            PRINT_NODE_POS(file, node);
            ASTNode_PrintNode((const ASTNode *) node->type_cast.expression, depth + 1, file);
            break;

        case AST_PRINT_STMT:
            PRINT_NODE_POS(file, node);
            for (const ASTNode *current = (ASTNode *) node->print_stmt.expressions; current != NULL;
                 current = (ASTNode *) current->next_node) {
                ASTNode_PrintNode(current, depth + 1, file);
            }
            break;

        case AST_MODULE:
            putc(' ', file);
            FPRINT_2COLOR(file, ANSI_BLACK_FG, ANSI_WHITE_BG,
                          node->module.module_name ? (char *) node->module.module_name : "<unnamed>");
            PRINT_NODE_POS(file, node);

            if (node->module.functions) {
                print_indent(file, depth + 1);
                FPRINT_3COLOR(file, ANSI_WHITE_FG, ANSI_BLACK_BG, ANSI_UNDERLINE, "functions");
                fprintf(file, "\n");
                for (const ASTNode *current = (ASTNode *) node->module.functions; current != NULL;
                     current = (ASTNode *) current->next_node) {
                    ASTNode_PrintNode(current, depth + 2, file);
                }
            }
            if (node->module.statements) {
                print_indent(file, depth + 1);
                FPRINT_3COLOR(file, ANSI_WHITE_FG, ANSI_BLACK_BG, ANSI_UNDERLINE, "statements");
                fprintf(file, "\n");
                for (const ASTNode *current = (ASTNode *) node->module.statements; current != NULL;
                     current = (ASTNode *) current->next_node) {
                    ASTNode_PrintNode(current, depth + 2, file);
                }
            }
            break;
        case AST_UNKNOWN:
            fprintf(file, " \n");
            break;
        case AST_FUNCTION_DECL:
            putc(' ', file);
            FPRINT_COLOR(file, ANSI_BRIGHT_YELLOW_FG, (const char*)node->function_decl.signature->function_name);
            PRINT_EXPR_TYPE(file, node->function_decl.signature->return_type);
            putc(' ', file);
            FPRINT_COLOR(file, ANSI_BRIGHT_BLACK_FG, "[");
            const size_t params_count = node->function_decl.signature->params.params_count;
            for (size_t i = 0; i < params_count; ++i) {
                PRINT_FUNCTION_PARAM(file, node->function_decl.signature->params);
                if (i + 1 < params_count) {
                    FPRINT_COLOR(file, ANSI_BRIGHT_BLACK_FG, ", ");
                }
            }
            FPRINT_COLOR(file, ANSI_BRIGHT_BLACK_FG, "]");
            putc('\n', file);
            ASTNode_PrintNode((const ASTNode *) node->function_decl.body, depth + 1, file);
            break;
        case AST_FUNCTION_CALL: {
            putc(' ', file);
            FPRINT_COLOR(file, ANSI_BRIGHT_YELLOW_FG, (const char*)node->function_call.signature->function_name);
            PRINT_EXPR_TYPE(file, node->function_call.expr_type);

            putc(' ', file);
            FPRINT_COLOR(file, ANSI_BRIGHT_BLACK_FG, "[");
            const size_t arguments_count = node->function_decl.signature->params.params_count;
            for (size_t i = 0; i < arguments_count; ++i) {
                PRINT_FUNCTION_PARAM(file, node->function_decl.signature->params);
                if (i + 1 < arguments_count) {
                    FPRINT_COLOR(file, ANSI_BRIGHT_BLACK_FG, ", ");
                }
            }
            FPRINT_COLOR(file, ANSI_BRIGHT_BLACK_FG, "]");
            fputc('\n', file);
            const ASTNode *argument = (const ASTNode *) node->function_call.arguments;
            for (const ASTNode *current = argument; current != NULL;
                 current = (const ASTNode *) current->next_node) {
                ASTNode_PrintNode(current, depth + 1, file);
            }
        }
        break;
        case AST_COUNT:
        default:
            fprintf(file, " <unhandled node type>\n");
            break;
    }
}

FunctionSignature *FindFunctionSignature(const ASTNode *module, const uint8_t *function_name) {
    DEBUG_ASSERT(module->type == AST_MODULE);

    const uint32_t function_name_hash = murmurhash3_string(function_name, MURMURHASH3_DEFAULT_STR_SEED);

    const ASTNode *functions = (const ASTNode *) module->module.functions;
    for (const ASTNode *current = functions; current != NULL; current = (const ASTNode *) current->next_node) {
        if (current->function_decl.signature->function_name_hash == function_name_hash
            && strcmp((const char *) current->function_decl.signature->function_name, (const char *) function_name) == 0
        ) {
            return current->function_decl.signature;
        }
    }

    return NULL;
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

ASTNode *CreateWhileStatementNode(Arena *arena, const Position pos, const ASTNode *condition, const ASTNode *body) {
    ASTNode *node = Arena_Type(arena, ASTNode);
    *node = (ASTNode){
        .type = AST_WHILE_STMT,
        .pos = pos,
        .next_node = NULL,
        .while_stmt = {
            .condition = (struct ASTNode *) condition,
            .body = (struct ASTNode *) body,
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

ASTNode *CreateFunctionDeclarationNode(Arena *arena, const Position pos, FunctionSignature *signature, ASTNode *body,
                                       Scope *scope) {
    ASTNode *node = Arena_Type(arena, ASTNode);
    *node = (ASTNode){
        .type = AST_FUNCTION_DECL,
        .pos = pos,
        .next_node = NULL,
        .function_decl = {
            .signature = signature,
            .body = (struct ASTNode *) body,
            .scope = scope,
        },
    };
    return node;
}
