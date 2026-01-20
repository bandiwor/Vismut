#include "codegen.h"

#include <stdarg.h>

CodeGenContext CodeGen_CreateContext(FILE *output) {
    return (CodeGenContext){
        .output = output,
    };
}

static void CodeGen_GenerateExpression(CodeGenContext ctx, const ASTNode *node);

static void CodeGen_GenerateStatement(CodeGenContext ctx, const ASTNode *node, int indent_level);

static void CodeGen_EmitIndent(const CodeGenContext ctx, const int indent_level) {
    for (int i = 0; i < indent_level; i++) {
        fprintf(ctx.output, "    ");
    }
}

static void CodeGen_Emit(const CodeGenContext ctx, const wchar_t *line) {
    fwprintf(ctx.output, L"%ls", line);
}

static void CodeGen_EmitFormat(const CodeGenContext ctx, const wchar_t *line, ...) {
    va_list args;
    va_start(args, line);
    vfwprintf(ctx.output, line, args);
    va_end(args);
}


static void CodeGen_EmitLine(const CodeGenContext ctx, const int indent_level, const wchar_t *fmt) {
    CodeGen_EmitIndent(ctx, indent_level);
    fwprintf(ctx.output, L"%ls\n", fmt);
}

// static void CodeGen_EmitLineFormat(const CodeGenContext *ctx, const wchar_t *fmt, ...) {
//     CodeGen_EmitIndent(ctx);
//
//     va_list args;
//     va_start(args, fmt);
//     vfwprintf_s(ctx->output, fmt, args);
//     va_end(args);
//
//     fwprintf_s(ctx->output, L"\n");
// }

static void CodeGen_GenerateLiteral(const CodeGenContext ctx, const ASTNode *node) {
    DEBUG_ASSERT(node->type == AST_LITERAL);

    switch (node->literal.type) {
        case VALUE_I64:
            CodeGen_EmitFormat(ctx, L"%lldLL", node->literal.i64);
            break;
        case VALUE_F64:
            if (node->literal.f64 != node->literal.f64) {
                CodeGen_Emit(ctx, L"NAN");
            } else if (node->literal.f64 == 1.0 / 0.0) {
                CodeGen_Emit(ctx, L"INFINITY");
            } else if (node->literal.f64 == -1.0 / 0.0) {
                CodeGen_Emit(ctx, L"-INFINITY");
            } else {
                CodeGen_EmitFormat(ctx, L"%.17g", node->literal.f64);
            }
            break;
        case VALUE_WSTR:
            CodeGen_Emit(ctx, L"L\"");
            for (const wchar_t *ptr = node->literal.wstr; *ptr != '\0'; ++ptr) {
                switch (*ptr) {
                    case L'\n':
                        CodeGen_Emit(ctx, L"\\n");
                        break;
                    case L'\r':
                        CodeGen_Emit(ctx, L"\\r");
                        break;
                    case L'\b':
                        CodeGen_Emit(ctx, L"\\b");
                        break;
                    case L'\v':
                        CodeGen_Emit(ctx, L"\\v");
                        break;
                    case L'\t':
                        CodeGen_Emit(ctx, L"\\t");
                        break;
                    default:
                        CodeGen_EmitFormat(ctx, L"%lc", *ptr);
                }
            }
            CodeGen_Emit(ctx, L"\"");
            break;
        default:
            break;
    }
}

static void CodeGen_GenerateBinaryOp(const CodeGenContext ctx, const ASTNode *node) {
    DEBUG_ASSERT(node->type == AST_BINARY);

    const wchar_t *op_str = NULL;
    switch (node->binary_op.op) {
        case AST_BINARY_ADD:
            op_str = L"+";
            break;
        case AST_BINARY_SUB:
            op_str = L"-";
            break;
        case AST_BINARY_MUL:
            op_str = L"*";
            break;
        case AST_BINARY_DIV:
            op_str = L"/";
            break;
        case AST_BINARY_ASSIGN:
            op_str = L"=";
            break;
        case AST_BINARY_MOD:
            op_str = L"%";
            break;
        case AST_BINARY_POW:
            CodeGen_Emit(ctx, L"pow(");
            CodeGen_GenerateExpression(ctx, (const ASTNode *) node->binary_op.left);
            CodeGen_Emit(ctx, L", ");
            CodeGen_GenerateExpression(ctx, (const ASTNode *) node->binary_op.right);
            CodeGen_Emit(ctx, L")");
            return;
        case AST_BINARY_EQUALS:
            op_str = L"==";
            break;
        case AST_BINARY_NOT_EQUALS:
            op_str = L"!=";
            break;
        case AST_BINARY_LESS_THAN:
            op_str = L"<";
            break;
        case AST_BINARY_LESS_THAN_OR_EQUALS:
            op_str = L"<=";
            break;
        case AST_BINARY_GREATER_THAN:
            op_str = L">";
            break;
        case AST_BINARY_GREATER_THAN_OR_EQUALS:
            op_str = L">=";
            break;
        case AST_BINARY_LOGICAL_AND:
            op_str = L"&&";
            break;
        case AST_BINARY_BITWISE_AND:
            op_str = L"&";
            break;
        case AST_BINARY_LOGICAL_OR:
            op_str = L"||";
            break;
        case AST_BINARY_BITWISE_OR:
            op_str = L"|";
            break;
        default:
            CodeGen_EmitFormat(ctx, L"/* unknown binary op '%ls' */", ASTBinaryType_String(node->binary_op.op));
            return;
    }

    //  ((<left>) <op> (<right>))
    CodeGen_Emit(ctx, L"((");
    CodeGen_GenerateExpression(ctx, (const ASTNode *) node->binary_op.left);
    CodeGen_EmitFormat(ctx, L") %ls (", op_str);
    CodeGen_GenerateExpression(ctx, (const ASTNode *) node->binary_op.right);
    CodeGen_Emit(ctx, L"))");
}

static void CodeGen_GenerateUnaryOp(const CodeGenContext ctx, const ASTNode *node) {
    DEBUG_ASSERT(node->type == AST_UNARY);

    const wchar_t *op_str = NULL;
    switch (node->unary_op.op) {
        case AST_UNARY_PLUS:
            op_str = L"+";
            break;
        case AST_UNARY_MINUS:
            op_str = L"-";
            break;
        case AST_UNARY_LOGICAL_NOT:
            op_str = L"!";
            break;
        case AST_UNARY_BITWISE_NOT:
            op_str = L"~";
            break;
        case AST_UNARY_INCREMENT:
            op_str = L"++";
            break;
        case AST_UNARY_DECREMENT:
            op_str = L"--";
            break;
        default:
            CodeGen_EmitFormat(ctx, L"/* unknown unary op '%ls' */", ASTUnaryType_String(node->unary_op.op));
            return;
    }

    //  (<op>(<expr>))
    CodeGen_EmitFormat(ctx, L"(%ls(", op_str);
    CodeGen_GenerateExpression(ctx, (const ASTNode *) node->unary_op.operand);
    CodeGen_Emit(ctx, L"))");
}

static const wchar_t *CodeGen_CTypeString(const VValueType type) {
    switch (type) {
        case VALUE_VOID:
            return L"void";
        case VALUE_I64:
            return L"long long";
        case VALUE_F64:
            return L"double";
        case VALUE_WSTR:
            return L"wchar_t*";
        default:
            return VValueType_String(type);
    }
}

static void CodeGen_GenerateTypeCast(const CodeGenContext ctx, const ASTNode *node) {
    DEBUG_ASSERT(node->type == AST_TYPE_CAST);

    //  ((<target>)(<expr>))
    CodeGen_EmitFormat(ctx, L"((%ls)(", CodeGen_CTypeString(node->type_cast.target_type));
    CodeGen_GenerateExpression(ctx, (ASTNode *) node->type_cast.expression);
    CodeGen_Emit(ctx, L"))");
}

static void CodeGen_GenerateTernary(const CodeGenContext ctx, const ASTNode *node) {
    DEBUG_ASSERT(node->type == AST_TERNARY);

    // ((<condition>) ? (<then>) : (<else>))
    CodeGen_Emit(ctx, L"((");
    CodeGen_GenerateExpression(ctx, (const ASTNode *) node->ternary_op.condition);
    CodeGen_Emit(ctx, L") ? (");
    CodeGen_GenerateExpression(ctx, (const ASTNode *) node->ternary_op.then_expression);
    CodeGen_Emit(ctx, L") : (");
    CodeGen_GenerateExpression(ctx, (const ASTNode *) node->ternary_op.else_expression);
    CodeGen_Emit(ctx, L"))");
}

static void CodeGen_GenerateExpression(const CodeGenContext ctx, const ASTNode *node) {
    switch (node->type) {
        case AST_LITERAL:
            CodeGen_GenerateLiteral(ctx, node);
            break;
        case AST_VAR_REF:
            CodeGen_EmitFormat(ctx, L"%ls", node->var_ref.var_name);
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
        default:
            CodeGen_EmitFormat(ctx, L"/* unknown expression, typeof = '%ls' */", ASTNodeType_String(node->type));
            break;
    }
}

static void CodeGen_GenerateVarDeclaration(const CodeGenContext ctx, const ASTNode *node, const int indent_level) {
    DEBUG_ASSERT(node->type == AST_VAR_DECL);

    const wchar_t *c_var_type = CodeGen_CTypeString(node->var_decl.var_type);
    const wchar_t *var_name = node->var_decl.var_name;
    const ASTNode *init_expr = (const ASTNode *) node->var_decl.init_value;

    CodeGen_EmitIndent(ctx, indent_level);
    if (init_expr == NULL) {
        // <ctype> <var_name>;
        CodeGen_EmitFormat(ctx, L"%ls %ls;\n", c_var_type, var_name);
        return;
    }

    CodeGen_EmitFormat(ctx, L"%ls %ls = ", c_var_type, var_name);
    CodeGen_GenerateExpression(ctx, init_expr);
    CodeGen_Emit(ctx, L";\n");
}

static void CodeGen_GenerateIfStatement(const CodeGenContext ctx, const ASTNode *node, const int indent_level) {
    DEBUG_ASSERT(node->type == AST_IF_STMT);

    const ASTNode *condition = (const ASTNode *) node->if_stmt.condition;
    const ASTNode *then_block = (const ASTNode *) node->if_stmt.then_block;
    const ASTNode *else_block = (const ASTNode *) node->if_stmt.else_block;

    // if (<condition>)
    CodeGen_EmitIndent(ctx, indent_level);
    CodeGen_Emit(ctx, L"if (");
    CodeGen_GenerateExpression(ctx, condition);
    CodeGen_Emit(ctx, L")\n");
    // then block
    CodeGen_GenerateStatement(ctx, then_block, then_block->type != AST_BLOCK ? indent_level + 1 : indent_level);
    // else block
    if (else_block == NULL) return;
    CodeGen_EmitLine(ctx, indent_level, L"else");
    CodeGen_GenerateStatement(ctx, else_block, else_block->type != AST_BLOCK ? indent_level + 1 : indent_level);
}

static void CodeGen_GenerateBlock(const CodeGenContext ctx, const ASTNode *node, const int indent_level) {
    DEBUG_ASSERT(node->type == AST_BLOCK);

    const ASTNode *statement = (const ASTNode *) node->block.statements;

    CodeGen_EmitLine(ctx, indent_level, L"{");
    for (const ASTNode *current = statement; current != NULL; current = (const ASTNode *) current->next_node) {
        CodeGen_GenerateStatement(ctx, current, indent_level + 1);
    }
    CodeGen_EmitLine(ctx, indent_level, L"}");
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
        default:
            return VALUE_UNKNOWN;
    }
}

static const wchar_t *GetNodePrintfFormat(const ASTNode *node) {
    switch (GetNodeExpressionType(node)) {
        case VALUE_I64:
            return L"%lld";
        case VALUE_F64:
            return L"%f";
        case VALUE_WSTR:
            return L"%ls";
        default:
            return L"";
    }
}

static void CodeGen_GeneratePrintStatement(const CodeGenContext ctx, const ASTNode *node, const int indent_level) {
    DEBUG_ASSERT(node->type == AST_PRINT_STMT);

    const ASTNode *expressions = (const ASTNode *) node->print_stmt.expressions;
    for (const ASTNode *current = expressions; current != NULL; current = (const ASTNode *) current->next_node) {
        CodeGen_EmitIndent(ctx, indent_level);
        CodeGen_EmitFormat(ctx, L"printf(\"%ls\", ", GetNodePrintfFormat(current));
        CodeGen_GenerateExpression(ctx, current);
        CodeGen_Emit(ctx, L");\n");
    }
}

static void CodeGen_GenerateStatement(const CodeGenContext ctx, const ASTNode *node, const int indent_level) {
    switch (node->type) {
        case AST_VAR_DECL:
            CodeGen_GenerateVarDeclaration(ctx, node, indent_level);
            break;
        case AST_IF_STMT:
            CodeGen_GenerateIfStatement(ctx, node, indent_level);
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
            CodeGen_Emit(ctx, L";\n");
            break;
    }
}

static void CodeGen_GeneratePrelude(const CodeGenContext ctx) {
    CodeGen_EmitLine(ctx, 0, L"#include <stdlib.h>");
    CodeGen_EmitLine(ctx, 0, L"#include <stdio.h>");
    CodeGen_EmitLine(ctx, 0, L"#include <math.h>");
    CodeGen_Emit(ctx, L"\n");
}

static void CodeGen_GenerateMain(const CodeGenContext ctx, const ASTNode *statement) {
    CodeGen_EmitLine(ctx, 0, L"int main(int argc, const char** argv) {");
    for (const ASTNode *current = statement; current != NULL; current = (const ASTNode *) current->next_node) {
        CodeGen_GenerateStatement(ctx, current, 1);
    }
    CodeGen_EmitLine(ctx, 1, L"return 0;");
    CodeGen_EmitLine(ctx, 0, L"}");
}

void CodeGen_GenerateFromAST(const CodeGenContext ctx, const ASTNode *module) {
    DEBUG_ASSERT(module->type == AST_MODULE);

    CodeGen_GeneratePrelude(ctx);
    CodeGen_GenerateMain(ctx, (const ASTNode *) module->module.statements);
}
