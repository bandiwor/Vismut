#include "codegen.h"

#include <stdarg.h>
#include <stdlib.h>

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

static void CodeGen_Emit(const CodeGenContext ctx, const char *line) {
    fprintf(ctx.output, "%s", line);
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
            CodeGen_Emit(ctx, "L\"");
            for (const char *ptr = node->literal.str; *ptr != '\0'; ++ptr) {
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
                        CodeGen_EmitFormat(ctx, "%lc", *ptr);
                }
            }
            CodeGen_Emit(ctx, "\"");
            break;
        default:
            break;
    }
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

    //  ((<left>) <op> (<right>))
    CodeGen_Emit(ctx, "((");
    CodeGen_GenerateExpression(ctx, (const ASTNode *) node->binary_op.left);
    CodeGen_EmitFormat(ctx, ") %s (", op_str);
    CodeGen_GenerateExpression(ctx, (const ASTNode *) node->binary_op.right);
    CodeGen_Emit(ctx, "))");
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
    CodeGen_EmitFormat(ctx, "(%s(", op_str);
    CodeGen_GenerateExpression(ctx, (const ASTNode *) node->unary_op.operand);
    CodeGen_Emit(ctx, "))");
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
            return "char*";
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
        default:
            CodeGen_EmitFormat(ctx, "/* unknown expression, typeof = '%s' */", ASTNodeType_String(node->type));
            break;
    }
}

static void CodeGen_GenerateVarDeclaration(const CodeGenContext ctx, const ASTNode *node, const int indent_level) {
    DEBUG_ASSERT(node->type == AST_VAR_DECL);

    const char *c_var_type = CodeGen_CTypeString(node->var_decl.var_type);
    const char *var_name = node->var_decl.var_name;
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

static void CodeGen_GeneratePrintStatement(const CodeGenContext ctx, const ASTNode *node, const int indent_level) {
    DEBUG_ASSERT(node->type == AST_PRINT_STMT);

    const ASTNode *expressions = (const ASTNode *) node->print_stmt.expressions;
    for (const ASTNode *current = expressions; current != NULL; current = (const ASTNode *) current->next_node) {
        CodeGen_EmitIndent(ctx, indent_level);
        CodeGen_EmitFormat(ctx, "printf(\"%s\", ", GetNodePrintfFormat(current));
        CodeGen_GenerateExpression(ctx, current);
        CodeGen_Emit(ctx, ");\n");
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
            CodeGen_Emit(ctx, ";\n");
            break;
    }
}

static void CodeGen_GeneratePrelude(const CodeGenContext ctx) {
    CodeGen_EmitLine(ctx, 0, "#include <stdlib.h>");
    CodeGen_EmitLine(ctx, 0, "#include <stdio.h>");
    CodeGen_EmitLine(ctx, 0, "#include <math.h>");
    CodeGen_Emit(ctx, "\n");
}

static void CodeGen_GenerateMain(const CodeGenContext ctx, const ASTNode *statement) {
    CodeGen_EmitLine(ctx, 0, "int main(int argc, const char** argv) {");
    for (const ASTNode *current = statement; current != NULL; current = (const ASTNode *) current->next_node) {
        CodeGen_GenerateStatement(ctx, current, 1);
    }
    CodeGen_EmitLine(ctx, 1, "return 0;");
    CodeGen_EmitLine(ctx, 0, "}");
}

void CodeGen_GenerateFromAST(const CodeGenContext ctx, const ASTNode *module) {
    DEBUG_ASSERT(module->type == AST_MODULE);

    CodeGen_GeneratePrelude(ctx);
    CodeGen_GenerateMain(ctx, (const ASTNode *) module->module.statements);
}
