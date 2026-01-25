#include "codegen.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

CodeGenContext CodeGen_CreateContext(FILE *output, const uint8_t* module_name) {
    return (CodeGenContext){
        .output = output,
        .module_name = module_name,
    };
}

static void CodeGen_GenerateExpression(CodeGenContext ctx, const ASTNode *node);

static void CodeGen_GenerateStatement(CodeGenContext ctx, const ASTNode *node, int indent_level);

static void CodeGen_EmitIndent(const CodeGenContext ctx, const int indent_level) {
    for (int i = 0; i < indent_level; i++) {
        fprintf(ctx.output, "    ");
    }
}

static void CodeGen_Emit(const CodeGenContext ctx, const char *line) {
    fprintf(ctx.output, "%s", line);
}

static void CodeGen_EmitSymbol(const CodeGenContext ctx, const uint8_t symbol) {
    putc(symbol, ctx.output);
}

static void CodeGen_EmitFormat(const CodeGenContext ctx, const char *line, ...) {
    va_list args;
    va_start(args, line);
    vfprintf(ctx.output, line, args);
    va_end(args);
}

static void CodeGen_EmitLine(const CodeGenContext ctx, const int indent_level, const char *fmt) {
    CodeGen_EmitIndent(ctx, indent_level);
    fprintf(ctx.output, "%s\n", fmt);
}

static void CodeGen_EmitGlobalName(const CodeGenContext ctx, const uint8_t *name) {
    fprintf(ctx.output, "_%s__%s", (const char *) ctx.module_name, (const char *) name);
}

static void CodeGen_GenerateLiteral(const CodeGenContext ctx, const ASTNode *node) {
    DEBUG_ASSERT(node->type == AST_LITERAL);

    switch (node->literal.type) {
        case VALUE_I64:
            CodeGen_EmitFormat(ctx, "%lldL", node->literal.i64);
            break;
        case VALUE_F64:
            if (node->literal.f64 != node->literal.f64) {
                CodeGen_Emit(ctx, "NAN");
            } else if (node->literal.f64 == 1.0 / 0.0) {
                CodeGen_Emit(ctx, "INFINITY");
            } else if (node->literal.f64 == -1.0 / 0.0) {
                CodeGen_Emit(ctx, "-INFINITY");
            } else {
                CodeGen_EmitFormat(ctx, "%.17g", node->literal.f64);
            }
            break;
        case VALUE_STR:
            CodeGen_Emit(ctx, "\"");
            for (const uint8_t *ptr = node->literal.str; *ptr != '\0'; ++ptr) {
                switch (*ptr) {
                    case L'\n':
                        CodeGen_Emit(ctx, "\\n");
                        break;
                    case L'\r':
                        CodeGen_Emit(ctx, "\\r");
                        break;
                    case L'\b':
                        CodeGen_Emit(ctx, "\\b");
                        break;
                    case L'\v':
                        CodeGen_Emit(ctx, "\\v");
                        break;
                    case L'\t':
                        CodeGen_Emit(ctx, "\\t");
                        break;
                    default:
                        CodeGen_EmitFormat(ctx, "%c", *ptr);
                }
            }
            CodeGen_Emit(ctx, "\"");
            break;
        default:
            break;
    }
}

static bool need_to_wrap_node(const ASTNode *node) {
    switch (node->type) {
        case AST_LITERAL:
        case AST_VAR_REF:
            return false;
        case AST_UNARY:
            return node->unary_op.op != AST_UNARY_BITWISE_NOT && node->unary_op.op != AST_UNARY_LOGICAL_NOT;
        default:
            return true;
    }
}

static void CodeGen_GenerateWrappedExpression(const CodeGenContext ctx, const ASTNode *node) {
    const bool is_need_to_wrap = need_to_wrap_node(node);
    if (is_need_to_wrap) CodeGen_Emit(ctx, "(");
    CodeGen_GenerateExpression(ctx, node);
    if (is_need_to_wrap) CodeGen_Emit(ctx, ")");
}

static void CodeGen_GenerateBinaryOp(const CodeGenContext ctx, const ASTNode *node) {
    DEBUG_ASSERT(node->type == AST_BINARY);

    const char *op_str = NULL;
    switch (node->binary_op.op) {
        case AST_BINARY_ADD:
            op_str = "+";
            break;
        case AST_BINARY_SUB:
            op_str = "-";
            break;
        case AST_BINARY_MUL:
            op_str = "*";
            break;
        case AST_BINARY_DIV:
            op_str = "/";
            break;
        case AST_BINARY_ASSIGN:
            op_str = "=";
            break;
        case AST_BINARY_MOD:
            op_str = "%";
            break;
        case AST_BINARY_POW:
            CodeGen_Emit(ctx, "pow(");
            CodeGen_GenerateExpression(ctx, (const ASTNode *) node->binary_op.left);
            CodeGen_Emit(ctx, ", ");
            CodeGen_GenerateExpression(ctx, (const ASTNode *) node->binary_op.right);
            CodeGen_Emit(ctx, ")");
            return;
        case AST_BINARY_EQUALS:
            op_str = "==";
            break;
        case AST_BINARY_NOT_EQUALS:
            op_str = "!=";
            break;
        case AST_BINARY_LESS_THAN:
            op_str = "<";
            break;
        case AST_BINARY_LESS_THAN_OR_EQUALS:
            op_str = "<=";
            break;
        case AST_BINARY_GREATER_THAN:
            op_str = ">";
            break;
        case AST_BINARY_GREATER_THAN_OR_EQUALS:
            op_str = ">=";
            break;
        case AST_BINARY_LOGICAL_AND:
            op_str = "&&";
            break;
        case AST_BINARY_BITWISE_AND:
            op_str = "&";
            break;
        case AST_BINARY_LOGICAL_OR:
            op_str = "||";
            break;
        case AST_BINARY_BITWISE_OR:
            op_str = "|";
            break;
        default:
            CodeGen_EmitFormat(ctx, "/* unknown binary op '%s' */", ASTBinaryType_String(node->binary_op.op));
            return;
    }

    const ASTNode *left = (const ASTNode *) node->binary_op.left;
    const ASTNode *right = (const ASTNode *) node->binary_op.right;

    CodeGen_Emit(ctx, "(");
    CodeGen_GenerateWrappedExpression(ctx, left);
    CodeGen_EmitFormat(ctx, " %s ", op_str);
    CodeGen_GenerateWrappedExpression(ctx, right);
    CodeGen_Emit(ctx, ")");
}

static void CodeGen_GenerateUnaryOp(const CodeGenContext ctx, const ASTNode *node) {
    DEBUG_ASSERT(node->type == AST_UNARY);

    const char *op_str = NULL;
    switch (node->unary_op.op) {
        case AST_UNARY_PLUS:
            op_str = "+";
            break;
        case AST_UNARY_MINUS:
            op_str = "-";
            break;
        case AST_UNARY_LOGICAL_NOT:
            op_str = "!";
            break;
        case AST_UNARY_BITWISE_NOT:
            op_str = "~";
            break;
        case AST_UNARY_INCREMENT:
            op_str = "++";
            break;
        case AST_UNARY_DECREMENT:
            op_str = "--";
            break;
        default:
            CodeGen_EmitFormat(ctx, "/* unknown unary op '%s' */", ASTUnaryType_String(node->unary_op.op));
            return;
    }

    //  (<op>(<expr>))
    const bool is_need_to_wrap = need_to_wrap_node(node);
    if (is_need_to_wrap) CodeGen_Emit(ctx, "(");
    CodeGen_Emit(ctx, op_str);
    CodeGen_GenerateWrappedExpression(ctx, (const ASTNode *) node->unary_op.operand);
    if (is_need_to_wrap) CodeGen_Emit(ctx, ")");
}

static const char *CodeGen_CTypeString(const VValueType type) {
    switch (type) {
        case VALUE_VOID:
            return "void";
        case VALUE_I64:
            return "long long";
        case VALUE_F64:
            return "double";
        case VALUE_STR:
            return "uint8_t*";
        default:
            return VValueType_String(type);
    }
}

static void CodeGen_GenerateTypeCast(const CodeGenContext ctx, const ASTNode *node) {
    DEBUG_ASSERT(node->type == AST_TYPE_CAST);

    //  ((<target>)(<expr>))
    CodeGen_EmitFormat(ctx, "((%s)(", CodeGen_CTypeString(node->type_cast.target_type));
    CodeGen_GenerateExpression(ctx, (ASTNode *) node->type_cast.expression);
    CodeGen_Emit(ctx, "))");
}

static void CodeGen_GenerateTernary(const CodeGenContext ctx, const ASTNode *node) {
    DEBUG_ASSERT(node->type == AST_TERNARY);

    // ((<condition>) ? (<then>) : (<else>))
    CodeGen_Emit(ctx, "((");
    CodeGen_GenerateExpression(ctx, (const ASTNode *) node->ternary_op.condition);
    CodeGen_Emit(ctx, ") ? (");
    CodeGen_GenerateExpression(ctx, (const ASTNode *) node->ternary_op.then_expression);
    CodeGen_Emit(ctx, ") : (");
    CodeGen_GenerateExpression(ctx, (const ASTNode *) node->ternary_op.else_expression);
    CodeGen_Emit(ctx, "))");
}

static void CodeGen_GenerateFunctionCall(const CodeGenContext ctx, const ASTNode *node) {
    CodeGen_EmitGlobalName(ctx, node->function_call.signature->function_name);
    CodeGen_EmitSymbol(ctx, '(');
    const ASTNode *argument = (const ASTNode *) node->function_call.arguments;
    for (const ASTNode *current = argument; current != NULL; current = (const ASTNode *) current->next_node) {
        CodeGen_GenerateExpression(ctx, current);
        if (current->next_node != NULL) {
            CodeGen_Emit(ctx, ", ");
        }
    }
    CodeGen_EmitSymbol(ctx, ')');
}

static void CodeGen_GenerateExpression(const CodeGenContext ctx, const ASTNode *node) {
    switch (node->type) {
        case AST_LITERAL:
            CodeGen_GenerateLiteral(ctx, node);
            break;
        case AST_VAR_REF:
            CodeGen_EmitFormat(ctx, "%s", node->var_ref.var_name);
            break;
        case AST_TYPE_CAST:
            CodeGen_GenerateTypeCast(ctx, node);
            break;
        case AST_UNARY:
            CodeGen_GenerateUnaryOp(ctx, node);
            break;
        case AST_BINARY:
            CodeGen_GenerateBinaryOp(ctx, node);
            break;
        case AST_TERNARY:
            CodeGen_GenerateTernary(ctx, node);
            break;
        case AST_FUNCTION_CALL:
            CodeGen_GenerateFunctionCall(ctx, node);
            break;
        default:
            CodeGen_EmitFormat(ctx, "/* unknown expression, typeof = '%s' */", ASTNodeType_String(node->type));
            break;
    }
}

static void CodeGen_GenerateVarDeclaration(const CodeGenContext ctx, const ASTNode *node, const int indent_level) {
    DEBUG_ASSERT(node->type == AST_VAR_DECL);

    const char *c_var_type = CodeGen_CTypeString(node->var_decl.var_type);
    const uint8_t *var_name = node->var_decl.var_name;
    const ASTNode *init_expr = (const ASTNode *) node->var_decl.init_value;

    CodeGen_EmitIndent(ctx, indent_level);
    if (init_expr == NULL) {
        // <ctype> <var_name>;
        CodeGen_EmitFormat(ctx, "%s %s;\n", c_var_type, var_name);
        return;
    }

    CodeGen_EmitFormat(ctx, "%s %s = ", c_var_type, var_name);
    CodeGen_GenerateExpression(ctx, init_expr);
    CodeGen_Emit(ctx, ";\n");
}

static void CodeGen_GenerateIfStatement(const CodeGenContext ctx, const ASTNode *node, const int indent_level) {
    DEBUG_ASSERT(node->type == AST_IF_STMT);

    const ASTNode *condition = (const ASTNode *) node->if_stmt.condition;
    const ASTNode *then_block = (const ASTNode *) node->if_stmt.then_block;
    const ASTNode *else_block = (const ASTNode *) node->if_stmt.else_block;

    // if (<condition>)
    CodeGen_EmitIndent(ctx, indent_level);
    CodeGen_Emit(ctx, "if (");
    CodeGen_GenerateExpression(ctx, condition);
    CodeGen_Emit(ctx, ")\n");
    // then block
    CodeGen_GenerateStatement(ctx, then_block, then_block->type != AST_BLOCK ? indent_level + 1 : indent_level);
    // else block
    if (else_block == NULL) return;
    CodeGen_EmitLine(ctx, indent_level, "else");
    CodeGen_GenerateStatement(ctx, else_block, else_block->type != AST_BLOCK ? indent_level + 1 : indent_level);
}

static void CodeGen_GenerateBlock(const CodeGenContext ctx, const ASTNode *node, const int indent_level) {
    DEBUG_ASSERT(node->type == AST_BLOCK);

    const ASTNode *statement = (const ASTNode *) node->block.statements;

    CodeGen_EmitLine(ctx, indent_level, "{");
    for (const ASTNode *current = statement; current != NULL; current = (const ASTNode *) current->next_node) {
        CodeGen_GenerateStatement(ctx, current, indent_level + 1);
    }
    CodeGen_EmitLine(ctx, indent_level, "}");
}

static VValueType GetNodeExpressionType(const ASTNode *node) {
    switch (node->type) {
        case AST_LITERAL:
            return node->literal.type;
        case AST_VAR_REF:
            return node->var_ref.expr_type;
        case AST_UNARY:
            return node->unary_op.expr_type;
        case AST_BINARY:
            return node->binary_op.expr_type;
        case AST_TERNARY:
            return node->ternary_op.expr_type;
        case AST_FUNCTION_CALL:
            return node->function_call.expr_type;
        default:
            return VALUE_UNKNOWN;
    }
}

static const char *GetNodePrintfFormat(const ASTNode *node) {
    switch (GetNodeExpressionType(node)) {
        case VALUE_I64:
            return "%lld";
        case VALUE_F64:
            return "%f";
        case VALUE_STR:
            return "%s";
        default:
            return "";
    }
}

static void CodeGen_GenerateLiteralForPrintf(const CodeGenContext ctx, const ASTNode *node) {
    DEBUG_ASSERT(node->type == AST_LITERAL);

    switch (node->literal.type) {
        case VALUE_I64:
            fprintf(ctx.output, "%lld", node->literal.i64);
            break;
        case VALUE_F64:
            fprintf(ctx.output, "%.17g", node->literal.f64);
            break;
        case VALUE_STR: {
            const uint8_t *ptr = node->literal.str;
            const uint8_t *end_ptr = ptr + strlen((const char *) ptr);
            while (ptr != end_ptr) {
                const uint8_t c = *ptr;
                switch (c) {
                    case '\n':
                        CodeGen_Emit(ctx, "\\n");
                        break;
                    case '\r':
                        CodeGen_Emit(ctx, "\\r");
                        break;
                    case '\t':
                        CodeGen_Emit(ctx, "\\t");
                        break;
                    case '\b':
                        CodeGen_Emit(ctx, "\\b");
                        break;
                    case '\v':
                        CodeGen_Emit(ctx, "\\v");
                        break;
                    case '\'':
                        CodeGen_Emit(ctx, "\'");
                        break;
                    case '\"':
                        CodeGen_Emit(ctx, "\"");
                        break;
                    case '\\':
                        CodeGen_Emit(ctx, "\\\\");
                        break;
                    default:
                        CodeGen_EmitSymbol(ctx, c);
                }
                ++ptr;
            }
        }
        default:
            break;
    }
}

static void CodeGen_GeneratePrintStatement(const CodeGenContext ctx, const ASTNode *node, const int indent_level) {
    DEBUG_ASSERT(node->type == AST_PRINT_STMT);

    CodeGen_EmitIndent(ctx, indent_level);
    const ASTNode *expressions = (const ASTNode *) node->print_stmt.expressions;

    CodeGen_Emit(ctx, "printf(\"");

    for (const ASTNode *current = expressions; current != NULL; current = (const ASTNode *) current->next_node) {
        if (current->type == AST_LITERAL) {
            CodeGen_GenerateLiteralForPrintf(ctx, current);
        } else {
            CodeGen_Emit(ctx, GetNodePrintfFormat(current));
        }
    }
    CodeGen_EmitSymbol(ctx, '\"');

    for (const ASTNode *current = expressions; current != NULL; current = (const ASTNode *) current->next_node) {
        if (current->type != AST_LITERAL) {
            CodeGen_Emit(ctx, ", ");
            CodeGen_GenerateExpression(ctx, current);
        }
    }

    CodeGen_Emit(ctx, ");\n");
}

static void CodeGen_GenerateWhileStatement(const CodeGenContext ctx, const ASTNode *node, const int indent_level) {
    DEBUG_ASSERT(node->type == AST_WHILE_STMT);

    const ASTNode *condition = (const ASTNode *) node->while_stmt.condition;
    const ASTNode *body = (const ASTNode *) node->while_stmt.body;
    CodeGen_EmitIndent(ctx, indent_level);
    CodeGen_Emit(ctx, "while (");
    CodeGen_GenerateExpression(ctx, condition);
    CodeGen_Emit(ctx, ")\n");
    CodeGen_GenerateStatement(ctx, body, body->type != AST_BLOCK ? indent_level + 1 : indent_level);
}

static void CodeGen_GenerateStatement(const CodeGenContext ctx, const ASTNode *node, const int indent_level) {
    switch (node->type) {
        case AST_VAR_DECL:
            CodeGen_GenerateVarDeclaration(ctx, node, indent_level);
            break;
        case AST_IF_STMT:
            CodeGen_GenerateIfStatement(ctx, node, indent_level);
            break;
        case AST_WHILE_STMT:
            CodeGen_GenerateWhileStatement(ctx, node, indent_level);
            break;
        case AST_BLOCK:
            CodeGen_GenerateBlock(ctx, node, indent_level);
            break;
        case AST_PRINT_STMT:
            CodeGen_GeneratePrintStatement(ctx, node, indent_level);
            break;
        default:
            CodeGen_EmitIndent(ctx, indent_level);
            CodeGen_GenerateExpression(ctx, node);
            CodeGen_Emit(ctx, ";\n");
            break;
    }
}

static void CodeGen_GeneratePrelude(const CodeGenContext ctx) {
    CodeGen_EmitLine(ctx, 0, "#include <stdlib.h>");
    CodeGen_EmitLine(ctx, 0, "#include <stdio.h>");
    CodeGen_EmitLine(ctx, 0, "#include <math.h>");
    CodeGen_EmitLine(ctx, 0, "#ifdef _WIN32");
    CodeGen_EmitLine(ctx, 1, "#include <Windows.h>");
    CodeGen_EmitLine(ctx, 1, "#define _VISMUT_ENABLE_UTF_WIN32");
    CodeGen_EmitLine(ctx, 0, "#endif");
    CodeGen_Emit(ctx, "\n");
}

static void CodeGen_GenerateSignature(const CodeGenContext ctx, const FunctionSignature *signature) {
    CodeGen_Emit(ctx, "static ");
    CodeGen_Emit(ctx, CodeGen_CTypeString(signature->return_type));
    CodeGen_EmitSymbol(ctx, ' ');
    CodeGen_EmitGlobalName(ctx, signature->function_name);
    CodeGen_EmitSymbol(ctx, '(');
    const size_t params_count = signature->params.params_count;
    for (size_t i = 0; i < params_count; ++i) {
        CodeGen_Emit(ctx, CodeGen_CTypeString(signature->params.param_types[i]));
        CodeGen_EmitSymbol(ctx, ' ');
        CodeGen_Emit(ctx, (const char *) signature->params.param_names[i]);
        if (i + 1 < params_count) {
            CodeGen_Emit(ctx, ", ");
        }
    }
    CodeGen_Emit(ctx, ")");
}

static void CodeGen_GenerateModuleFunctionsSignatures(const CodeGenContext ctx, const ASTNode *functions) {
    for (const ASTNode *current = functions; current != NULL; current = (const ASTNode *) current->next_node) {
        CodeGen_GenerateSignature(ctx, current->function_decl.signature);
        CodeGen_Emit(ctx, ";\n\n");
    }

    CodeGen_Emit(ctx, "\n");
}

static void CodeGen_GenerateModuleFunctionsDeclarations(const CodeGenContext ctx, const ASTNode *functions) {
    for (const ASTNode *current = functions; current != NULL; current = (const ASTNode *) current->next_node) {
        CodeGen_GenerateSignature(ctx, current->function_decl.signature);
        if (((const ASTNode *) current->function_decl.body)->type != AST_BLOCK) {
            CodeGen_Emit(ctx, " {\n");
            CodeGen_EmitIndent(ctx, 1);
            CodeGen_Emit(ctx, "return ");
            CodeGen_GenerateExpression(ctx, (const ASTNode *) current->function_decl.body);
            CodeGen_Emit(ctx, ";\n}\n\n");
            continue;
        }
        CodeGen_EmitSymbol(ctx, '\n');
        CodeGen_GenerateBlock(ctx, (const ASTNode *) current->function_decl.body, 0);
        CodeGen_EmitSymbol(ctx, '\n');
    }

    CodeGen_Emit(ctx, "\n");
}

static void CodeGen_GenerateMain(const CodeGenContext ctx, const ASTNode *statement) {
    CodeGen_EmitLine(ctx, 0, "int main(int argc, const char **argv) {");
    CodeGen_EmitLine(ctx, 0, "#ifdef _VISMUT_ENABLE_UTF_WIN32");
    CodeGen_EmitLine(ctx, 1, "SetConsoleOutputCP(CP_UTF8);");
    CodeGen_EmitLine(ctx, 1, "SetConsoleCP(CP_UTF8);");
    CodeGen_EmitLine(ctx, 0, "#endif");
    CodeGen_EmitLine(ctx, 1, "");

    for (const ASTNode *current = statement; current != NULL; current = (const ASTNode *) current->next_node) {
        CodeGen_GenerateStatement(ctx, current, 1);
    }

    CodeGen_EmitLine(ctx, 1, "");
    CodeGen_EmitLine(ctx, 1, "return 0;");
    CodeGen_EmitLine(ctx, 0, "}\n");
}

void CodeGen_GenerateFromAST(const CodeGenContext ctx, const ASTNode *module) {
    DEBUG_ASSERT(module->type == AST_MODULE);

    CodeGen_GeneratePrelude(ctx);
    CodeGen_GenerateModuleFunctionsSignatures(ctx, (const ASTNode *) module->module.functions);
    CodeGen_GenerateMain(ctx, (const ASTNode *) module->module.statements);
    CodeGen_GenerateModuleFunctionsDeclarations(ctx, (const ASTNode *) module->module.functions);
}
