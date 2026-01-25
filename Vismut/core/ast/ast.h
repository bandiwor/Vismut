//
// Created by kir on 23.11.2025.
//

#ifndef VISMUT_AST_H
#define VISMUT_AST_H
#include "../types.h"
#include "value.h"
#include "scope.h"
#include <stdbool.h>

typedef struct FunctionParamNode {
    struct FunctionParamNode *next;
    const uint8_t *name;
    VValueType type;
} FunctionParamNode;

typedef struct {
    const uint8_t **param_names;
    VValueType *param_types;
    size_t params_count;
} FunctionParams;

typedef struct {
    FunctionParams params;
    const uint8_t *function_name;
    uint32_t function_name_hash;
    VValueType return_type;
    int flags;
} FunctionSignature;

typedef struct {
    ASTNodeType type;
    Position pos;
    struct ASTNode *next_node;

    union {
        VValue literal;

        struct {
            const uint8_t *var_name;
            VValueType expr_type;
        } var_ref;

        struct {
            const uint8_t *var_name;
            VValueType var_type;
            VValueType init_value_type;
            struct ASTNode *init_value;
        } var_decl;

        struct {
            FunctionSignature *signature;
            const struct ASTNode *arguments;
            VValueType expr_type;
            size_t arguments_count;
        } function_call;

        struct {
            FunctionSignature *signature;
            const struct ASTNode *body;
            Scope *scope;
        } function_decl;

        struct {
            struct ASTNode *operand;
            VValueType expr_type;
            ASTUnaryType op;
            bool is_pure;
        } unary_op;

        struct {
            struct ASTNode *left;
            struct ASTNode *right;
            VValueType expr_type;
            ASTBinaryType op;
            bool is_pure;
        } binary_op;

        struct {
            struct ASTNode *condition;
            struct ASTNode *then_expression;
            struct ASTNode *else_expression;
            VValueType expr_type;
            bool is_pure;
        } ternary_op;

        struct {
            struct ASTNode *condition;
            struct ASTNode *then_block;
            struct ASTNode *else_block;
        } if_stmt;

        struct {
            struct ASTNode *condition;
            struct ASTNode *body;
        } while_stmt;

        struct {
            struct ASTNode *statements;
            Scope *scope;
        } block;

        struct {
            struct ASTNode *statements;
            struct ASTNode *functions;
            const uint8_t *module_name;
            Scope *scope;
        } module;

        struct {
            struct ASTNode *expression;
            VValueType target_type;
            VValueType from_type;
            bool is_explicit;
            bool is_pure;
        } type_cast;

        struct {
            struct ASTNode *expressions;
        } print_stmt;

        struct {
            struct ASTNode *variable;
        } input_stmt;
    };
} ASTNode;

void ASTNode_Print(const ASTNode *, FILE *);

attribute_pure
FunctionSignature *FindFunctionSignature(const ASTNode *module, const uint8_t *function_name);

ASTNode *CreateLiteralNode(Arena *arena, Position pos, VValue value);

ASTNode *CreateVarRefNode(Arena *arena, Position pos, const uint8_t *var_name);

ASTNode *CreateBinaryNode(Arena *arena, Position pos, const ASTNode *left, const ASTNode *right,
                          ASTBinaryType op, bool is_pure);

ASTNode *CreateUnaryNode(Arena *arena, Position pos, const ASTNode *operand, ASTUnaryType op,
                         bool is_pure);

ASTNode *CreateIfStatementNode(Arena *arena, Position pos, const ASTNode *condition,
                               const ASTNode *then_block, const ASTNode *else_block);

ASTNode *CreateWhileStatementNode(Arena *arena, Position pos, const ASTNode *condition, const ASTNode *body);

ASTNode *CreateVarDeclarationNode(Arena *arena, Position pos, const uint8_t *var_name,
                                  VValueType var_type, const ASTNode *init_value);

ASTNode *CreateTypeCastNode(Arena *arena, Position pos, ASTNode *expression,
                            VValueType target_type/*, const bool is_explicit*/);

ASTNode *CreateTernaryNode(Arena *arena, Position pos, const ASTNode *condition,
                           const ASTNode *then_expression, const ASTNode *else_expression);

ASTNode *CreateModuleNode(Arena *arena, const uint8_t *module_name, Scope *scope);

ASTNode *CreateBlockNode(Arena *arena, Position pos, ASTNode *statements, Scope *scope);

ASTNode *CreatePrintStatementNode(Arena *arena, Position pos, ASTNode *expressions);

ASTNode *CreateFunctionDeclarationNode(Arena *arena, Position pos, FunctionSignature *signature, ASTNode *body,
                                       Scope *scope);

#endif //VISMUT_AST_H
