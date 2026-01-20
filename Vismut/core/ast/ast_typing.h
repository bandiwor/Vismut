//
// Created by kir on 25.12.2025.
//

#ifndef VISMUT_AST_TYPING_H
#define VISMUT_AST_TYPING_H
#include <stdbool.h>

#include "../types.h"

attribute_const
VValueType GetBinaryOpResultType(ASTBinaryType op, VValueType left, VValueType right);

attribute_const
VValueType GetUnaryOpResultType(ASTUnaryType op, VValueType operand);

attribute_const
bool IsCastAllowed(VValueType from_type, VValueType to_type, bool is_explicit);

attribute_const
VValueType FindCommonType(VValueType type_1, VValueType type_2);

#endif //VISMUT_AST_TYPING_H
