#include "token.h"

const char *token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "XATO";
        case TOKEN_AMAL: return "amal";
        case TOKEN_QAYTAR: return "qaytar";
        case TOKEN_AGAR: return "agar";
        case TOKEN_AKS: return "aks";
        case TOKEN_TOKI: return "toki";
        case TOKEN_UCHUN: return "uchun";
        case TOKEN_CHIQAR: return "chiqar";
        case TOKEN_KIRIT: return "kirit";
        case TOKEN_TOXTAT: return "to'xtat";
        case TOKEN_DAVOM: return "davom";
        case TOKEN_TUZILMA: return "tuzilma";
        case TOKEN_YANGI: return "yangi";
        case TOKEN_UZUNLIK: return "uzunlik";
        case TOKEN_VA: return "va";
        case TOKEN_YOKI: return "yoki";
        case TOKEN_INKOR: return "inkor";
        case TOKEN_IDENT: return "IDENT";
        case TOKEN_INT: return "INT";
        case TOKEN_STRING: return "STRING";
        case TOKEN_LBRACE: return "{";
        case TOKEN_RBRACE: return "}";
        case TOKEN_LPAREN: return "(";
        case TOKEN_RPAREN: return ")";
        case TOKEN_LBRACKET: return "[";
        case TOKEN_RBRACKET: return "]";
        case TOKEN_COMMA: return ",";
        case TOKEN_SEMICOLON: return ";";
        case TOKEN_DOT: return ".";
        case TOKEN_COLON: return ":";
        case TOKEN_ASSIGN: return "=";
        case TOKEN_PLUS: return "+";
        case TOKEN_MINUS: return "-";
        case TOKEN_STAR: return "*";
        case TOKEN_SLASH: return "/";
        case TOKEN_PERCENT: return "%";
        case TOKEN_EQ: return "==";
        case TOKEN_NEQ: return "!=";
        case TOKEN_LT: return "<";
        case TOKEN_LTE: return "<=";
        case TOKEN_GT: return ">";
        case TOKEN_GTE: return ">=";
        default: return "NOMA'LUM";
    }
}
