#ifndef TOKEN_H
#define TOKEN_H

#include <stdint.h>

typedef enum {
    TOKEN_EOF = 0,
    TOKEN_ERROR,

    // Kalit so'zlar (Uzbek Keywords)
    TOKEN_AMAL,       // amal (funksiya)
    TOKEN_QAYTAR,     // qaytar (return)
    TOKEN_AGAR,       // agar (if)
    TOKEN_AKS,        // aks (else)
    TOKEN_TOKI,       // toki (while)
    TOKEN_CHIQAR,     // chiqar (print)
    TOKEN_TOXTAT,     // toxtat (break)
    TOKEN_DAVOM,      // davom (continue)

    // Literallar va identifikatorlar
    TOKEN_IDENT,      // o'zgaruvchi yoki funksiya nomi
    TOKEN_INT,        // 64-bitli butun son
    TOKEN_STRING,     // matn ("...")

    // Belgilar va qavslar
    TOKEN_LBRACE,     // {
    TOKEN_RBRACE,     // }
    TOKEN_LPAREN,     // (
    TOKEN_RPAREN,     // )
    TOKEN_COMMA,      // ,
    TOKEN_SEMICOLON,  // ;

    // Operatorlar
    TOKEN_ASSIGN,     // =
    TOKEN_PLUS,       // +
    TOKEN_MINUS,      // -
    TOKEN_STAR,       // *
    TOKEN_SLASH,      // /
    TOKEN_PERCENT,    // %

    // Taqqoslash operatorlari
    TOKEN_EQ,         // ==
    TOKEN_NEQ,        // !=
    TOKEN_LT,         // <
    TOKEN_LTE,        // <=
    TOKEN_GT,         // >
    TOKEN_GTE,        // >=
} TokenType;

typedef struct {
    TokenType type;
    char *text;
    int length;
    int64_t int_val;
    int line;
    int col;
} Token;

const char *token_type_to_string(TokenType type);

#endif // TOKEN_H
