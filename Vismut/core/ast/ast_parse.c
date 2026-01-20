#include "ast_parse.h"

#include <stdlib.h>

#include "../errors/errors.h"
#include "../errors/callstack.h"

#define CURRENT_TOKEN_TYPE(ast_parser_ptr) ((ast_parser_ptr)->current_token.type)
#define CURRENT_TOKEN_POS(ast_parser_ptr) ((ast_parser_ptr)->current_token.position)

#define CURRENT_TOKEN_TYPE_ASSERT(ast_parser_ptr, token_type)    \
    START_BLOCK_WRAPPER \
        if (CURRENT_TOKEN_TYPE(ast_parser_ptr) != (token_type)) { \
            return VISMUT_ERROR_UNEXPECTED_TOKEN;                 \
        } \
    END_BLOCK_WRAPPER

#define NEXT_TOKEN_SAFE(ast_parser_ptr, err_var) RISKY_EXPRESSION_SAFE(Tokenizer_Next((ast_parser_ptr)->tokenizer, &(ast_parser_ptr)->current_token), err_var)

#define PARSE_EXPRESSION_SAFE(ast_parser_ptr, err_var, node_ptr_ptr) RISKY_EXPRESSION_SAFE(ASTParser_ParseExpression(ast_parser_ptr, node_ptr_ptr), err_var)

#define PARSE_EXPRESSION_WITH_PRECEDENCE_SAFE(ast_parser_ptr, err_var, node_ptr_ptr, precedence) RISKY_EXPRESSION_SAFE(ASTParser_ParseExpressionWithPrecedence(ast_parser_ptr, node_ptr_ptr, precedence), err_var)

#define PARSE_EXPRESSION_OR_BLOCK_SAFE(ast_parser_ptr, err_var, node_ptr_ptr) RISKY_EXPRESSION_SAFE(ASTParser_ParseExpressionOrBlock(ast_parser_ptr, node_ptr_ptr), err_var)

#define NEXT_TOKEN_EXCEPT(ast_parser_ptr, err_var, token_type) \
    START_BLOCK_WRAPPER \
        NEXT_TOKEN_SAFE(ast_parser_ptr, err_var); \
        CURRENT_TOKEN_TYPE_ASSERT(ast_parser_ptr, token_type);\
    END_BLOCK_WRAPPER

typedef enum {
    PRECEDENCE_NONE,
    PRECEDENCE_MINIMAL,
    PRECEDENCE_ASSIGNMENT = PRECEDENCE_MINIMAL,
    PRECEDENCE_TERNARY,
    PRECEDENCE_LOGICAL_OR,
    PRECEDENCE_LOGICAL_AND,
    PRECEDENCE_EQUALITY,
    PRECEDENCE_RELATIONAL,
    PRECEDENCE_ADDITIVE,
    PRECEDENCE_MULTIPLICATIVE,
    PRECEDENCE_UNARY,
    PRECEDENCE_PRIMARY,
} OperatorPrecedence;

static errno_t ASTParser_ParseUnaryExpression(ASTParser *, ASTNode **);

static errno_t ASTParser_ParsePrimaryExpression(ASTParser *, ASTNode **);

static errno_t ASTParser_ParseExpression(ASTParser *, ASTNode **);

static errno_t ASTParser_ParseExpressionWithPrecedence(ASTParser *, ASTNode **, OperatorPrecedence);

static errno_t ASTParser_ParseStatement(ASTParser *ast_parser, ASTNode **node);

static errno_t ASTParser_ParseExpressionOrBlock(ASTParser *ast_parser, ASTNode **node);

static OperatorPrecedence GetPrecedence(const VTokenType token) {
    CALLSTACK_TRACE();
    switch (token) {
        case TOKEN_ASSIGN:
            return PRECEDENCE_ASSIGNMENT;
        case TOKEN_QUESTION:
            return PRECEDENCE_TERNARY;
        case TOKEN_LOGICAL_OR:
            return PRECEDENCE_LOGICAL_OR;
        case TOKEN_LOGICAL_AND:
            return PRECEDENCE_LOGICAL_AND;
        case TOKEN_EQUALS:
        case TOKEN_NOT_EQUALS:
            return PRECEDENCE_EQUALITY;
        case TOKEN_LESS_THAN:
        case TOKEN_LESS_THAN_OR_EQUALS:
        case TOKEN_GREATER_THAN:
        case TOKEN_GREATER_THAN_OR_EQUALS:
            return PRECEDENCE_RELATIONAL;
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            return PRECEDENCE_ADDITIVE;
        case TOKEN_STAR:
        case TOKEN_DIVIDE:
        case TOKEN_INT_DIVIDE:
            return PRECEDENCE_MULTIPLICATIVE;
        case TOKEN_POWER:
            return PRECEDENCE_UNARY;
        default:
            return PRECEDENCE_NONE;
    }
}

attribute_const
static ASTBinaryType GetBinaryType(const VTokenType token) {
    switch (token) {
        case TOKEN_PLUS:
            return AST_BINARY_ADD;
        case TOKEN_MINUS:
            return AST_BINARY_SUB;
        case TOKEN_STAR:
            return AST_BINARY_MUL;
        case TOKEN_DIVIDE:
            return AST_BINARY_DIV;
        case TOKEN_INT_DIVIDE:
            return AST_BINARY_INT_DIV;
        case TOKEN_MODULE_DIV:
            return AST_BINARY_MOD;
        case TOKEN_ASSIGN:
            return AST_BINARY_ASSIGN;
        case TOKEN_POWER:
            return AST_BINARY_POW;
        case TOKEN_EQUALS:
            return AST_BINARY_EQUALS;
        case TOKEN_NOT_EQUALS:
            return AST_BINARY_NOT_EQUALS;
        case TOKEN_GREATER_THAN:
            return AST_BINARY_GREATER_THAN;
        case TOKEN_GREATER_THAN_OR_EQUALS:
            return AST_BINARY_GREATER_THAN_OR_EQUALS;
        case TOKEN_LESS_THAN:
            return AST_BINARY_LESS_THAN;
        case TOKEN_LESS_THAN_OR_EQUALS:
            return AST_BINARY_LESS_THAN_OR_EQUALS;
        default:
            return AST_BINARY_UNKNOWN;
    }
}

attribute_const
static ASTUnaryType GetUnaryType(const VTokenType token) {
    switch (token) {
        case TOKEN_PLUS:
            return AST_UNARY_PLUS;
        case TOKEN_MINUS:
            return AST_UNARY_MINUS;
        case TOKEN_TILDA:
            return AST_UNARY_BITWISE_NOT;
        case TOKEN_EXCLAMATION_MARK:
            return AST_UNARY_LOGICAL_NOT;
        case TOKEN_INCREMENT:
            return AST_UNARY_INCREMENT;
        case TOKEN_DECREMENT:
            return AST_UNARY_DECREMENT;
        default:
            return AST_UNARY_UNKNOWN;
    }
}

static int IsRightAssocOperator(const ASTBinaryType token) {
    return token == AST_BINARY_POW || token == AST_BINARY_ASSIGN;
}

static errno_t ASTParser_ParseType(const VTokenType token, VValueType *out_value) {
    CALLSTACK_TRACE();

    VValueType value;
    switch (token) {
        case TOKEN_I64_TYPE:
            value = VALUE_I64;
            break;
        case TOKEN_FLOAT_TYPE:
            value = VALUE_F64;
            break;
        case TOKEN_STRING_TYPE:
            value = VALUE_STR;
            break;
        default:
            return VISMUT_ERROR_UNKNOWN_TYPE;
    }
    *out_value = value;
    return VISMUT_ERROR_OK;
}

static VValue GetVValueFromLiteralToken(const VToken token) {
    switch (token.type) {
        case TOKEN_INT_LITERAL:
            return (VValue){
                .type = VALUE_I64,
                .i64 = token.data.i64
            };
        case TOKEN_FLOAT_LITERAL:
            return (VValue){
                .type = VALUE_F64,
                .f64 = token.data.f64
            };
        case TOKEN_CHARS_LITERAL:
            return (VValue){
                .type = VALUE_STR,
                .str = token.data.chars
            };
        default:
            return (VValue){
                .type = VALUE_UNKNOWN,
            };
    }
}

static errno_t ASTParser_ParseLiteral(ASTParser *ast_parser, ASTNode **node) {
    CALLSTACK_TRACE();
    DEBUG_ASSERT(
        ast_parser->current_token.type == TOKEN_INT_LITERAL || ast_parser->current_token.type == TOKEN_FLOAT_LITERAL ||
        ast_parser->current_token.type == TOKEN_CHARS_LITERAL);
    errno_t err;

    const VValue value = GetVValueFromLiteralToken(ast_parser->current_token);
    if (value.type == VALUE_UNKNOWN) {
        return VISMUT_ERROR_UNEXPECTED_TOKEN;
    }

    *node = CreateLiteralNode(ast_parser->arena, ast_parser->current_token.position, value);

    NEXT_TOKEN_SAFE(ast_parser, err);
    return VISMUT_ERROR_OK;
}

static errno_t ASTParser_ParseVarRef(ASTParser *ast_parser, ASTNode **node) {
    CALLSTACK_TRACE();
    DEBUG_ASSERT(ast_parser->current_token.type == TOKEN_IDENTIFIER);
    errno_t err;

    *node = CreateVarRefNode(
        ast_parser->arena, ast_parser->current_token.position, ast_parser->current_token.data.chars
    );

    NEXT_TOKEN_SAFE(ast_parser, err);
    return VISMUT_ERROR_OK;
}

static errno_t ASTParser_ParseParenthesizedExpression(ASTParser *ast_parser, ASTNode **node) {
    CALLSTACK_TRACE();
    errno_t err;

    NEXT_TOKEN_SAFE(ast_parser, err); // Пропускаем '('

    PARSE_EXPRESSION_SAFE(ast_parser, err, node);

    CURRENT_TOKEN_TYPE_ASSERT(ast_parser, TOKEN_RPAREN);
    NEXT_TOKEN_SAFE(ast_parser, err); // Пропускаем ')'

    return VISMUT_ERROR_OK;
}

static errno_t ASTParser_ParseUnaryExpression(ASTParser *ast_parser, ASTNode **node) {
    CALLSTACK_TRACE();
    errno_t err;

    const VTokenType op = CURRENT_TOKEN_TYPE(ast_parser);

    if (op != TOKEN_PLUS && op != TOKEN_MINUS && op != TOKEN_EXCLAMATION_MARK && op !=
        TOKEN_TILDA) {
        return ASTParser_ParsePrimaryExpression(ast_parser, node);
    }

    const Position op_pos = ast_parser->current_token.position;
    NEXT_TOKEN_SAFE(ast_parser, err);

    ASTNode *operand = NULL;
    RISKY_EXPRESSION_SAFE(ASTParser_ParseUnaryExpression(ast_parser, &operand), err);

    const ASTUnaryType unary_op = GetUnaryType(op);

    *node = CreateUnaryNode(
        ast_parser->arena, op_pos, operand, unary_op,
        unary_op != AST_UNARY_INCREMENT && unary_op != AST_UNARY_DECREMENT
    );
    return VISMUT_ERROR_OK;
}

static errno_t ASTParser_ParseTypeCast(ASTParser *ast_parser, ASTNode **node) {
    CALLSTACK_TRACE();
    DEBUG_ASSERT(
        CURRENT_TOKEN_TYPE(ast_parser) == TOKEN_I64_TYPE || CURRENT_TOKEN_TYPE(ast_parser) == TOKEN_FLOAT_TYPE
    );
    errno_t err;
    const Position pos = ast_parser->current_token.position;

    VValueType target_type;
    RISKY_EXPRESSION_SAFE(ASTParser_ParseType(ast_parser->current_token.type, &target_type), err);

    NEXT_TOKEN_EXCEPT(ast_parser, err, TOKEN_LPAREN);
    NEXT_TOKEN_SAFE(ast_parser, err);

    ASTNode *expression;
    PARSE_EXPRESSION_SAFE(ast_parser, err, &expression);

    CURRENT_TOKEN_TYPE_ASSERT(ast_parser, TOKEN_RPAREN);
    NEXT_TOKEN_SAFE(ast_parser, err);

    *node = CreateTypeCastNode(
        ast_parser->arena, pos, expression, target_type /*, true*/
    );

    return VISMUT_ERROR_OK;
}

static errno_t ASTParser_ParsePrimaryExpression(
    ASTParser *ast_parser, ASTNode **node) {
    CALLSTACK_TRACE();

    switch (CURRENT_TOKEN_TYPE(ast_parser)) {
        case TOKEN_I64_TYPE:
        case TOKEN_FLOAT_TYPE:
            return ASTParser_ParseTypeCast(ast_parser, node);
        case TOKEN_INT_LITERAL:
        case TOKEN_FLOAT_LITERAL:
        case TOKEN_CHARS_LITERAL:
            return ASTParser_ParseLiteral(ast_parser, node);
        case TOKEN_IDENTIFIER:
            return ASTParser_ParseVarRef(ast_parser, node);
        case TOKEN_LPAREN:
            return ASTParser_ParseParenthesizedExpression(
                ast_parser, node);
        default:
            return VISMUT_ERROR_UNEXPECTED_TOKEN;
    }
}

static errno_t ASTParser_ParseExpressionWithPrecedence
(ASTParser *ast_parser, ASTNode **node, const OperatorPrecedence min_precedence) {
    CALLSTACK_TRACE();
    errno_t err;

    ASTNode *left = NULL;
    RISKY_EXPRESSION_SAFE(ASTParser_ParseUnaryExpression(ast_parser, &left), err);

    while (1) {
        const VTokenType op = CURRENT_TOKEN_TYPE(ast_parser);
        const OperatorPrecedence precedence = GetPrecedence(op);

        if (precedence < min_precedence) {
            break;
        }

        const Position op_pos = ast_parser->current_token.position;
        if (op == TOKEN_QUESTION) {
            NEXT_TOKEN_SAFE(ast_parser, err);

            ASTNode *then_expr = NULL;
            PARSE_EXPRESSION_SAFE(ast_parser, err, &then_expr);

            CURRENT_TOKEN_TYPE_ASSERT(ast_parser, TOKEN_COLON);
            NEXT_TOKEN_SAFE(ast_parser, err);

            ASTNode *else_expr = NULL;
            PARSE_EXPRESSION_WITH_PRECEDENCE_SAFE(
                ast_parser, err, &else_expr,
                PRECEDENCE_TERNARY - 1);

            left = CreateTernaryNode(
                ast_parser->arena, op_pos,
                left, then_expr, else_expr
            );
            continue;
        }

        NEXT_TOKEN_SAFE(ast_parser, err);

        const ASTBinaryType binary_op = GetBinaryType(op);
        const int is_right_assoc = IsRightAssocOperator(binary_op);

        ASTNode *right = NULL;
        PARSE_EXPRESSION_WITH_PRECEDENCE_SAFE(
            ast_parser, err, &right,
            precedence + (1 - is_right_assoc));

        left = CreateBinaryNode(
            ast_parser->arena, op_pos, left,
            right, binary_op,
            binary_op != AST_BINARY_ASSIGN
        );
    }

    *node = left;
    return VISMUT_ERROR_OK;
}

static errno_t ASTParser_ParseExpression(
    ASTParser *ast_parser, ASTNode **node) {
    return ASTParser_ParseExpressionWithPrecedence(ast_parser, node, PRECEDENCE_MINIMAL);
}

ASTParser ASTParser_Create(Tokenizer *tokenizer) {
    Scope *module_scope = Scope_Allocate(tokenizer->arena, NULL);
    ASTNode *module = CreateModuleNode(
        tokenizer->arena, tokenizer->source_filename, module_scope
    );

    return (ASTParser){
        .source = tokenizer->source,
        .source_length = tokenizer->source_length,
        .arena = tokenizer->arena,
        .tokenizer = tokenizer,
        .current_token = (VToken){0},
        .module_node = module,
        .current_scope = module_scope,
    };
}

static errno_t ASTParser_ParseNameDeclaration(ASTParser *ast_parser, ASTNode **node) {
    CALLSTACK_TRACE();
    DEBUG_ASSERT(ast_parser->current_token.type == TOKEN_NAME_DECLARATION);
    errno_t err;

    const Position pos = ast_parser->current_token.position;

    // Parse: var name
    NEXT_TOKEN_EXCEPT(ast_parser, err, TOKEN_IDENTIFIER);
    const char *var_name = ast_parser->current_token.data.chars;

    // Get next token TOKEN_ASSIGN or TOKEN_COLON
    NEXT_TOKEN_SAFE(ast_parser, err);
    switch (ast_parser->current_token.type) {
        case TOKEN_ASSIGN: {
            // Skip TOKEN_ASSIGN and set type to auto
            NEXT_TOKEN_SAFE(ast_parser, err);
            ASTNode *init_value;
            PARSE_EXPRESSION_SAFE(ast_parser, err, &init_value);
            *node = CreateVarDeclarationNode(
                ast_parser->arena,
                pos,
                var_name,
                VALUE_AUTO,
                init_value
            );
            return VISMUT_ERROR_OK;
        }
        case TOKEN_COLON: {
            NEXT_TOKEN_SAFE(ast_parser, err);
            VValueType variable_type;
            RISKY_EXPRESSION_SAFE(ASTParser_ParseType(ast_parser->current_token.type, &variable_type), err);
            NEXT_TOKEN_SAFE(ast_parser, err);
            if (ast_parser->current_token.type != TOKEN_ASSIGN) {
                *node = CreateVarDeclarationNode(
                    ast_parser->arena,
                    pos,
                    var_name,
                    variable_type,
                    NULL
                );
                return VISMUT_ERROR_OK;
            }
            NEXT_TOKEN_SAFE(ast_parser, err);
            ASTNode *init_value;
            PARSE_EXPRESSION_SAFE(ast_parser, err, &init_value);
            *node = CreateVarDeclarationNode(
                ast_parser->arena,
                pos,
                var_name,
                variable_type,
                init_value
            );
            return VISMUT_ERROR_OK;
        }
        default:
            return VISMUT_ERROR_UNEXPECTED_TOKEN;
    }
}

static errno_t ASTParser_ParseIfStatement(ASTParser *ast_parser, ASTNode **node) {
    CALLSTACK_TRACE();
    DEBUG_ASSERT(ast_parser->current_token.type == TOKEN_CONDITION_STATEMENT);
    errno_t err;

    const Position pos = ast_parser->current_token.position;
    NEXT_TOKEN_SAFE(ast_parser, err);

    ASTNode *condition;
    PARSE_EXPRESSION_SAFE(ast_parser, err, &condition);

    ASTNode *then_block;
    PARSE_EXPRESSION_OR_BLOCK_SAFE(ast_parser, err, &then_block);

    ASTNode *else_block = NULL;
    if (CURRENT_TOKEN_TYPE(ast_parser) == TOKEN_EXCLAMATION_MARK) {
        NEXT_TOKEN_SAFE(ast_parser, err);
        PARSE_EXPRESSION_OR_BLOCK_SAFE(ast_parser, err, &else_block);
    } else if (CURRENT_TOKEN_TYPE(ast_parser) == TOKEN_CONDITION_ELSE_IF) {
        // replace: #! -> #
        ast_parser->current_token.type = TOKEN_CONDITION_STATEMENT;
        RISKY_EXPRESSION_SAFE(ASTParser_ParseIfStatement(ast_parser, &else_block), err);
    }

    *node = CreateIfStatementNode(
        ast_parser->arena,
        pos,
        condition,
        then_block,
        else_block
    );

    return VISMUT_ERROR_OK;
}

static errno_t ASTParser_ParseBlock(ASTParser *ast_parser, ASTNode **node) {
    CALLSTACK_TRACE();
    DEBUG_ASSERT(ast_parser->current_token.type == TOKEN_LBRACE);
    errno_t err;

    const Position lbrace_pos = ast_parser->current_token.position;
    NEXT_TOKEN_SAFE(ast_parser, err);

    Scope *block_scope = Scope_Allocate(ast_parser->arena, ast_parser->current_scope);
    ast_parser->current_scope = block_scope;

    ASTNode *first_statement = NULL;
    ASTNode *last_statement = NULL;

    while (CURRENT_TOKEN_TYPE(ast_parser) != TOKEN_RBRACE) {
        ASTNode *statement = NULL;
        RISKY_EXPRESSION_SAFE(ASTParser_ParseStatement(ast_parser, &statement), err);
        if (statement == NULL) {
            printf("Statement is NULL. %s %d\n", __FILE__, __LINE__);
            exit(1);
        }

        if (first_statement == NULL) {
            first_statement = statement;
            last_statement = statement;
        } else {
            DEBUG_ASSERT(last_statement != NULL);
            last_statement->next_node = (struct ASTNode *) statement;
            last_statement = statement;
        }
    }

    const Position rbrace_pos = ast_parser->current_token.position;
    NEXT_TOKEN_SAFE(ast_parser, err);
    ast_parser->current_scope = ast_parser->current_scope->parent;

    *node = CreateBlockNode(
        ast_parser->arena, Position_Join(lbrace_pos, rbrace_pos), first_statement, block_scope
    );

    return VISMUT_ERROR_OK;
}

static errno_t ASTParser_ParseExpressionOrBlock(ASTParser *ast_parser, ASTNode **node) {
    CALLSTACK_TRACE();

    if (CURRENT_TOKEN_TYPE(ast_parser) == TOKEN_LBRACE) {
        return ASTParser_ParseBlock(ast_parser, node);
    }

    return ASTParser_ParseExpression(ast_parser, node);
}

static errno_t ASTParser_ParsePrintStatement(ASTParser *ast_parser, ASTNode **node) {
    CALLSTACK_TRACE();
    DEBUG_ASSERT(CURRENT_TOKEN_TYPE(ast_parser) == TOKEN_PRINT_STATEMENT);
    errno_t err;

    const Position pos = CURRENT_TOKEN_POS(ast_parser);
    NEXT_TOKEN_SAFE(ast_parser, err);

    ASTNode *first_expression = NULL;
    ASTNode *current_expression = NULL;

    while (CURRENT_TOKEN_TYPE(ast_parser) != TOKEN_EOF) {
        ASTNode *statement;
        RISKY_EXPRESSION_SAFE(ASTParser_ParseExpression(ast_parser, &statement), err);

        if (first_expression == NULL) {
            first_expression = statement;
            current_expression = statement;
        } else {
            DEBUG_ASSERT(current_expression != NULL);
            current_expression->next_node = (struct ASTNode *) statement;
            current_expression = statement;
        }
        if (CURRENT_TOKEN_TYPE(ast_parser) != TOKEN_COMMA) {
            break;
        }
        NEXT_TOKEN_SAFE(ast_parser, err);
    }

    *node = CreatePrintStatementNode(ast_parser->arena, pos, first_expression);

    return VISMUT_ERROR_OK;
}

static errno_t ASTParser_ParseStatement(ASTParser *ast_parser, ASTNode **node) {
    CALLSTACK_TRACE();
    errno_t err;

start_label:
    switch (CURRENT_TOKEN_TYPE(ast_parser)) {
        case TOKEN_NAME_DECLARATION:
            RISKY_EXPRESSION_SAFE(ASTParser_ParseNameDeclaration(ast_parser, node), err);
            goto clear_semicolon;
        case TOKEN_CONDITION_STATEMENT:
            RISKY_EXPRESSION_SAFE(ASTParser_ParseIfStatement(ast_parser, node), err);
            goto clear_semicolon;
        case TOKEN_PRINT_STATEMENT:
            RISKY_EXPRESSION_SAFE(ASTParser_ParsePrintStatement(ast_parser, node), err);
            goto clear_semicolon;;
        case TOKEN_SEMICOLON:
            NEXT_TOKEN_SAFE(ast_parser, err);
            goto start_label;
        default:
            RISKY_EXPRESSION_SAFE(ASTParser_ParseExpression(ast_parser, node), err);
            break;
    }
clear_semicolon:
    while (CURRENT_TOKEN_TYPE(ast_parser) == TOKEN_SEMICOLON) {
        NEXT_TOKEN_SAFE(ast_parser, err);
    }
    return err;
}

errno_t ASTParser_Parse(ASTParser *ast_parser) {
    DEBUG_ASSERT(ast_parser != NULL);
    CALLSTACK_TRACE();
    errno_t err;

    NEXT_TOKEN_SAFE(ast_parser, err);

    ASTNode *first_statement = NULL;
    ASTNode *last_statement = NULL;
    ASTNode *first_function = NULL;
    ASTNode *last_function = NULL;

    while (ast_parser->current_token.type != TOKEN_EOF) {
        ASTNode *statement = NULL;
        RISKY_EXPRESSION_SAFE(ASTParser_ParseStatement(ast_parser, &statement), err);
        if (statement->type == AST_FUNCTION_DECL) {
            if (first_function == NULL) {
                first_function = statement;
                last_function = statement;
            } else {
                DEBUG_ASSERT(last_function != NULL);
                last_function->next_node = (struct ASTNode *) statement;
                last_function = statement;
            }
        }

        if (first_statement == NULL) {
            first_statement = statement;
            last_statement = statement;
        } else {
            DEBUG_ASSERT(last_statement != NULL);
            last_statement->next_node = (struct ASTNode *) statement;
            last_statement = statement;
        }
    }

    ast_parser->module_node->module.statements = (struct ASTNode *) first_statement;
    ast_parser->module_node->module.functions = (struct ASTNode *) first_function;

    return VISMUT_ERROR_OK;
}
