//
// Created by kir on 23.11.2025.
//

#ifndef VISMUT_AST_H
#define VISMUT_AST_H
#include "../types.h"
#include "value.h"
#include "scope.h"
#include <stdbool.h>

typedef struct {
    ASTNodeType type;
    Position pos;
    struct ASTNode *next_node;

    union {
        VValue literal;

        struct {
            const char *var_name;
            VValueType expr_type;
        } var_ref;

        struct {
            const char *var_name;
            VValueType var_type;
            VValueType init_value_type;
            struct ASTNode *init_value;
        } var_decl;

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
            const char *module_name;
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
    };
} ASTNode;

void ASTNode_Print(const ASTNode *);

ASTNode *CreateLiteralNode(Arena *arena, Position pos, VValue value);

ASTNode *CreateVarRefNode(Arena *arena, Position pos, const char *var_name);

ASTNode *CreateBinaryNode(Arena *arena, Position pos, const ASTNode *left, const ASTNode *right,
                          ASTBinaryType op, bool is_pure);

ASTNode *CreateUnaryNode(Arena *arena, Position pos, const ASTNode *operand, ASTUnaryType op,
                         bool is_pure);

ASTNode *CreateIfStatementNode(Arena *arena, Position pos, const ASTNode *condition,
                               const ASTNode *then_block, const ASTNode *else_block);

ASTNode *CreateVarDeclarationNode(Arena *arena, Position pos, const char *var_name,
                                  VValueType var_type, const ASTNode *init_value);

ASTNode *CreateTypeCastNode(Arena *arena, Position pos, ASTNode *expression,
                            VValueType target_type/*, const bool is_explicit*/);

ASTNode *CreateTernaryNode(Arena *arena, Position pos, const ASTNode *condition,
                           const ASTNode *then_expression, const ASTNode *else_expression);

ASTNode *CreateModuleNode(Arena *arena, const char *module_name, Scope *scope);

ASTNode *CreateBlockNode(Arena *arena, Position pos, ASTNode *statements, Scope *scope);

ASTNode *CreatePrintStatementNode(Arena *arena, Position pos, ASTNode *expressions);

#endif //VISMUT_AST_H
